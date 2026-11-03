import base64
import hashlib
import http.client
import json
import os
from pathlib import Path
import shutil
import socket
import subprocess
import tempfile
import time
import unittest


BACKEND = Path(__file__).resolve().parents[1]
PASSWORD = "senha-de-teste"
SALT = b"0123456789abcdef"
ITERATIONS = 600_000


def password_hash(password):
    digest = hashlib.pbkdf2_hmac(
        "sha256", password.encode("utf-8"), SALT, ITERATIONS
    )
    encode = lambda value: base64.b64encode(value).decode("ascii").rstrip("=")
    return f"$pbkdf2-sha256${ITERATIONS}${encode(SALT)}${encode(digest)}"


class AuthenticationApiTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.build_directory = tempfile.TemporaryDirectory(prefix="auth-c-build-")
        cls.binary = Path(cls.build_directory.name) / (
            "auth-server.exe" if os.name == "nt" else "auth-server"
        )
        compiler = shutil.which("gcc")
        if not compiler:
            raise RuntimeError("gcc é necessário para compilar e testar o backend C")

        command = [
            compiler,
            "-std=c11",
            "-Wall",
            "-Wextra",
            "-Werror",
            str(BACKEND / "src" / "main.c"),
            str(BACKEND / "src" / "auth.c"),
            "-o",
            str(cls.binary),
        ]
        if os.name == "nt":
            command.append("-lws2_32")
        else:
            command.append("-lcrypto")

        subprocess.run(command, cwd=BACKEND, check=True)

    @classmethod
    def tearDownClass(cls):
        cls.build_directory.cleanup()

    def setUp(self):
        self.data_directory = tempfile.TemporaryDirectory(prefix="auth-c-data-")
        self.users_file = Path(self.data_directory.name) / "users.json"
        self.write_users(
            [
                {
                    "usuarioId": "usuario-123",
                    "identificador": "pessoa@example.com",
                    "senhaHash": password_hash(PASSWORD),
                },
                {
                    "usuarioId": "usuario-com-acento",
                    "identificador": "usuário@example.com",
                    "senhaHash": password_hash("senha-á"),
                },
            ]
        )
        self.start_server()

    def tearDown(self):
        self.stop_server()
        self.data_directory.cleanup()

    def write_users(self, users):
        self.users_file.write_text(
            json.dumps({"usuarios": users}),
            encoding="utf-8",
        )

    def start_server(self, users_json=None):
        with socket.socket() as sock:
            sock.bind(("127.0.0.1", 0))
            self.port = sock.getsockname()[1]

        environment = os.environ.copy()
        environment.update(
            {
                "AUTH_USERS_FILE": str(self.users_file),
                "HOST": "127.0.0.1",
                "PORT": str(self.port),
                "WEB_ROOT": str(BACKEND / "public"),
            }
        )
        if users_json is not None:
            environment["AUTH_USERS_JSON"] = users_json
        self.server = subprocess.Popen(
            [str(self.binary)],
            cwd=BACKEND,
            env=environment,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

        for _ in range(50):
            if self.server.poll() is not None:
                stdout, stderr = self.server.communicate()
                self.fail(f"servidor encerrou na inicialização: {stdout}{stderr}")
            try:
                connection = http.client.HTTPConnection(
                    "127.0.0.1", self.port, timeout=0.1
                )
                connection.request("GET", "/")
                response = connection.getresponse()
                response.read()
                connection.close()
                return
            except OSError:
                time.sleep(0.1)

        self.stop_server()
        self.fail("servidor não ficou disponível no tempo esperado")

    def stop_server(self):
        if getattr(self, "server", None) and self.server.poll() is None:
            self.server.terminate()
            self.last_stdout, self.last_stderr = self.server.communicate(timeout=5)
            self.server = None

    def restart_with_users(self, users):
        self.stop_server()
        self.write_users(users)
        self.start_server()

    def request(self, method, path, body=None, headers=None):
        connection = http.client.HTTPConnection("127.0.0.1", self.port, timeout=3)
        connection.request(method, path, body=body, headers=headers or {})
        response = connection.getresponse()
        payload = response.read()
        result = response.status, response.getheaders(), payload
        connection.close()
        return result

    def authenticate(self, body, content_type="application/json"):
        encoded_body = body if isinstance(body, bytes) else json.dumps(body).encode()
        headers = (
            {"Content-Type": content_type} if content_type is not None else {}
        )
        return self.request(
            "POST",
            "/autenticacao",
            encoded_body,
            headers,
        )

    def test_credenciais_validas_retorna_somente_a_identidade(self):
        status, headers, payload = self.authenticate(
            {"identificador": "pessoa@example.com", "senha": PASSWORD}
        )

        self.assertEqual(status, 200)
        self.assertEqual(json.loads(payload), {"usuarioId": "usuario-123"})
        self.assertIn(("Cache-Control", "no-store"), headers)

    def test_credenciais_com_caracteres_unicode(self):
        status, _, payload = self.authenticate(
            {"identificador": "usuário@example.com", "senha": "senha-á"}
        )

        self.assertEqual(status, 200)
        self.assertEqual(json.loads(payload), {"usuarioId": "usuario-com-acento"})

    def test_interface_estatica_e_servida_com_a_api(self):
        page_status, _, page = self.request("GET", "/")
        script_status, script_headers, script = self.request("GET", "/app.js")
        style_status, style_headers, _ = self.request("GET", "/style.css")

        self.assertEqual(page_status, 200)
        self.assertIn(b"lang=\"pt-BR\"", page)
        self.assertIn(b"/app.js", page)
        self.assertEqual(script_status, 200)
        self.assertIn(("Content-Type", "text/javascript; charset=utf-8"), script_headers)
        self.assertIn(b"fetch(\"/autenticacao\"", script)
        self.assertEqual(style_status, 200)
        self.assertIn(("Content-Type", "text/css; charset=utf-8"), style_headers)

    def test_autenticacao_usa_json_de_usuarios_configurado_no_ambiente(self):
        users_json = self.users_file.read_text(encoding="utf-8")
        self.stop_server()
        self.start_server(users_json=users_json)
        status, _, payload = self.authenticate(
            {"identificador": "pessoa@example.com", "senha": PASSWORD}
        )

        self.assertEqual(status, 200)
        self.assertEqual(json.loads(payload), {"usuarioId": "usuario-123"})

    def test_usuario_inexistente_e_senha_incorreta_retorna_o_mesmo_401(self):
        cases = [
            {"identificador": "inexistente@example.com", "senha": PASSWORD},
            {"identificador": "pessoa@example.com", "senha": "senha-incorreta"},
        ]
        results = []
        for credentials in cases:
            status, _, payload = self.authenticate(credentials)
            results.append((status, json.loads(payload)))

        self.assertEqual(results, [
            (
                401,
                {
                    "codigo": "AUTENTICACAO_INVALIDA",
                    "mensagem": "Identificador ou senha inválidos.",
                },
            ),
            (
                401,
                {
                    "codigo": "AUTENTICACAO_INVALIDA",
                    "mensagem": "Identificador ou senha inválidos.",
                },
            ),
        ])

    def test_senha_com_espacos_significativos_nao_e_normalizada(self):
        status, _, _ = self.authenticate(
            {"identificador": "pessoa@example.com", "senha": f" {PASSWORD} "}
        )
        self.assertEqual(status, 401)

    def test_entrada_invalida_retorna_400(self):
        cases = [
            {"senha": PASSWORD},
            {"identificador": "pessoa@example.com"},
            {"identificador": "  ", "senha": PASSWORD},
            {"identificador": "pessoa@example.com", "senha": "  "},
            {"identificador": 42, "senha": PASSWORD},
            {"identificador": "pessoa@example.com", "senha": True},
            {
                "identificador": "pessoa@example.com",
                "senha": PASSWORD,
                "campoExtra": "não permitido",
            },
        ]
        for body in cases:
            with self.subTest(body=body):
                status, _, payload = self.authenticate(body)
                self.assertEqual(status, 400)
                self.assertEqual(json.loads(payload)["codigo"], "REQUISICAO_INVALIDA")

    def test_json_malformado_ou_vazio_retorna_400(self):
        for body in [b"{", b""]:
            with self.subTest(body=body):
                status, _, _ = self.authenticate(body)
                self.assertEqual(status, 400)

    def test_corpo_acima_de_16_kib_retorna_413(self):
        status, _, payload = self.authenticate(b"x" * (16 * 1024 + 1))
        self.assertEqual(status, 413)
        self.assertEqual(json.loads(payload)["codigo"], "CORPO_MUITO_GRANDE")

    def test_tipo_de_conteudo_invalido_retorna_415(self):
        for content_type in ["text/plain", None]:
            with self.subTest(content_type=content_type):
                status, _, payload = self.authenticate(
                    {"identificador": "pessoa@example.com", "senha": PASSWORD},
                    content_type=content_type,
                )
                self.assertEqual(status, 415)
                self.assertEqual(
                    json.loads(payload)["codigo"], "TIPO_DE_CONTEUDO_INVALIDO"
                )

    def test_metodo_invalido_retorna_405_e_allow(self):
        status, headers, _ = self.request("GET", "/autenticacao")
        self.assertEqual(status, 405)
        self.assertIn(("Allow", "POST"), headers)

    def test_rota_inexistente_retorna_404(self):
        for path in ["/outro-recurso", "/usuarios"]:
            with self.subTest(path=path):
                status, _, _ = self.request("POST", path, b"{}")
                self.assertEqual(status, 404)

    def test_hash_invalido_retorna_500_e_nao_expoe_detalhes_no_log(self):
        user = {
            "usuarioId": "usuario-123",
            "identificador": "pessoa@example.com",
            "senhaHash": "hash inválido",
        }
        self.restart_with_users([user])
        status, _, payload = self.authenticate(
            {"identificador": user["identificador"], "senha": PASSWORD}
        )
        self.assertEqual(status, 500)
        self.assertEqual(json.loads(payload)["codigo"], "ERRO_INTERNO")
        self.stop_server()
        self.assertIn("falha_interna_de_autenticacao", self.last_stderr)
        self.assertNotIn(PASSWORD, self.last_stderr)
        self.assertNotIn(user["senhaHash"], self.last_stderr)

    def test_repositorio_invalido_impede_inicializacao(self):
        invalid_files = [
            "{",
            json.dumps(
                {
                    "usuarios": [
                        {
                            "usuarioId": "1",
                            "identificador": "duplicado@example.com",
                            "senhaHash": "hash-1",
                        },
                        {
                            "usuarioId": "2",
                            "identificador": "duplicado@example.com",
                            "senhaHash": "hash-2",
                        },
                    ]
                }
            ),
            json.dumps(
                {
                    "usuarios": [
                        {
                            "usuarioId": "duplicado",
                            "identificador": "uma@example.com",
                            "senhaHash": "hash-1",
                        },
                        {
                            "usuarioId": "duplicado",
                            "identificador": "outra@example.com",
                            "senhaHash": "hash-2",
                        },
                    ]
                }
            ),
            json.dumps(
                {
                    "usuarios": [
                        {
                            "usuarioId": "1",
                            "identificador": "pessoa@example.com",
                            "senhaHash": "hash",
                            "senha": PASSWORD,
                        }
                    ]
                }
            ),
        ]
        for contents in invalid_files:
            with self.subTest(contents=contents):
                self.users_file.write_text(contents, encoding="utf-8")
                environment = os.environ.copy()
                environment.update(
                    {
                        "AUTH_USERS_FILE": str(self.users_file),
                        "HOST": "127.0.0.1",
                        "PORT": str(self.port),
                        "WEB_ROOT": str(BACKEND / "public"),
                    }
                )
                result = subprocess.run(
                    [str(self.binary)],
                    cwd=BACKEND,
                    env=environment,
                    capture_output=True,
                    text=True,
                    timeout=5,
                )
                self.assertEqual(result.returncode, 1)
                self.assertIn("falha_de_inicializacao", result.stderr)
                self.assertNotIn(PASSWORD, result.stderr)

    def test_json_de_usuarios_invalido_no_ambiente_impede_inicializacao(self):
        environment = os.environ.copy()
        environment.update(
            {
                "AUTH_USERS_FILE": str(self.users_file),
                "AUTH_USERS_JSON": "{",
                "HOST": "127.0.0.1",
                "PORT": str(self.port),
            }
        )
        result = subprocess.run(
            [str(self.binary)],
            cwd=BACKEND,
            env=environment,
            capture_output=True,
            text=True,
            timeout=5,
        )

        self.assertEqual(result.returncode, 1)
        self.assertIn("falha_de_inicializacao", result.stderr)
        self.assertNotIn(PASSWORD, result.stderr)

    def test_arquivo_de_usuarios_ausente_impede_inicializacao(self):
        environment = os.environ.copy()
        environment.update(
            {
                "AUTH_USERS_FILE": str(Path(self.data_directory.name) / "missing.json"),
                "HOST": "127.0.0.1",
                "PORT": str(self.port),
            }
        )
        environment.pop("AUTH_USERS_JSON", None)
        result = subprocess.run(
            [str(self.binary)],
            cwd=BACKEND,
            env=environment,
            capture_output=True,
            text=True,
            timeout=5,
        )

        self.assertEqual(result.returncode, 1)
        self.assertIn("falha_de_inicializacao", result.stderr)
        self.assertNotIn(PASSWORD, result.stderr)


if __name__ == "__main__":
    unittest.main()
