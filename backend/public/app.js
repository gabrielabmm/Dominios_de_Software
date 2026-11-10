const form = document.querySelector("#authentication-form");
const result = document.querySelector("#resultado");
const submitButton = form.querySelector("button[type='submit']");

function showResult(message, state) {
  result.textContent = message;
  result.dataset.state = state;
}

form.addEventListener("submit", async (event) => {
  event.preventDefault();
  submitButton.disabled = true;
  showResult("Verificando credenciais...", "pending");

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
      showResult("Não foi possível conectar ao servidor.", "error");
      return;
    }

    let body;
    try {
      body = await response.json();
    } catch (error) {
      if (!(error instanceof SyntaxError)) {
        throw error;
      }
      showResult("O servidor retornou uma resposta inválida.", "error");
      return;
    }

    if (response.ok) {
      showResult(
        `Login realizado com sucesso! Usuário: ${body.usuarioId}`,
        "success",
      );
    } else {
      showResult(
        typeof body?.mensagem === "string" && body.mensagem
          ? body.mensagem
          : response.status === 401
            ? "Identificador ou senha inválidos."
            : "Não foi possível autenticar.",
        "error",
      );
    }
  } catch {
    showResult("Não foi possível concluir a autenticação.", "error");
  } finally {
    submitButton.disabled = false;
  }
});
