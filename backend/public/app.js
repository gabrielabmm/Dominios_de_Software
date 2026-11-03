const form = document.querySelector("#authentication-form");
const result = document.querySelector("#resultado");
const submitButton = form.querySelector("button[type='submit']");

form.addEventListener("submit", async (event) => {
  event.preventDefault();
  submitButton.disabled = true;
  result.textContent = "Verificando credenciais...";

  try {
    let response;
    try {
      response = await fetch("/autenticacao", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          identificador: form.elements.identificador.value,
          senha: form.elements.senha.value,
        }),
      });
    } catch (error) {
      if (!(error instanceof TypeError)) {
        throw error;
      }
      result.textContent = "Não foi possível conectar ao servidor.";
      return;
    }

    let body;
    try {
      body = await response.json();
    } catch (error) {
      if (!(error instanceof SyntaxError)) {
        throw error;
      }
      result.textContent = "O servidor retornou uma resposta inválida.";
      return;
    }

    if (response.ok) {
      result.textContent = `Autenticação bem-sucedida. Usuário: ${body.usuarioId}`;
    } else {
      result.textContent = body.mensagem ?? "Não foi possível autenticar.";
    }
  } finally {
    submitButton.disabled = false;
  }
});
