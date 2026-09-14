const themeToggle = document.getElementById("themeToggle");
const menuButton = document.getElementById("menuButton");
const nav = document.getElementById("nav");
const pageLabel = document.getElementById("pageLabel");

function aplicarTema(tema) {

    if (tema === "light") {

        document.body.classList.add("light");

        pageLabel.textContent = "referências - claro";

    } else {

        document.body.classList.remove("light");

        pageLabel.textContent = "referências - escuro";
    }
}

const temaSalvo = localStorage.getItem("mindmovie-tema") || "dark";

aplicarTema(temaSalvo);

themeToggle.addEventListener("click", function () {

    const temaAtual = document.body.classList.contains("light")
        ? "light"
        : "dark";

    const novoTema = temaAtual === "light"
        ? "dark"
        : "light";

    localStorage.setItem("mindmovie-tema", novoTema);

    aplicarTema(novoTema);
});

menuButton.addEventListener("click", function () {

    nav.classList.toggle("active");

});