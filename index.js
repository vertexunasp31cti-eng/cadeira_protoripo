document.addEventListener("DOMContentLoaded", () => {

    /* =========================================
       MODO CLARO / ESCURO
    ========================================= */

    const themeToggle = document.getElementById("themeToggle");
    const body = document.body;

    // Recupera o tema salvo
    const temaSalvo = localStorage.getItem("mindmovie-theme");

    // Aplica o tema salvo ao abrir a página
    if (temaSalvo === "light") {
        body.classList.add("light");
    } else {
        body.classList.remove("light");
    }

    // Clique no botão de tema
    if (themeToggle) {

        themeToggle.addEventListener("click", () => {

            body.classList.toggle("light");

            // Verifica qual tema está ativo
            const modoClaro = body.classList.contains("light");

            // Salva a escolha
            if (modoClaro) {
                localStorage.setItem("mindmovie-theme", "light");
            } else {
                localStorage.setItem("mindmovie-theme", "dark");
            }

        });

    }


    /* =========================================
       MENU MOBILE
    ========================================= */

    const botaoMenu = document.getElementById("botaoMenu");
    const menu = document.getElementById("menu");

    if (botaoMenu && menu) {

        botaoMenu.addEventListener("click", () => {

            menu.classList.toggle("aberto");

            const aberto = menu.classList.contains("aberto");

            botaoMenu.setAttribute(
                "aria-expanded",
                aberto ? "true" : "false"
            );

        });


        /* =========================================
           FECHAR MENU AO CLICAR EM UM LINK
        ========================================= */

        const linksMenu = menu.querySelectorAll("a");

        linksMenu.forEach((link) => {

            link.addEventListener("click", () => {

                menu.classList.remove("aberto");

                botaoMenu.setAttribute(
                    "aria-expanded",
                    "false"
                );

            });

        });

    }

});