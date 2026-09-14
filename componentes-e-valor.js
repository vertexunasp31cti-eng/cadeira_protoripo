document.addEventListener("DOMContentLoaded", function () {

    /* ========================================
       ELEMENTOS
    ======================================== */

    const botaoMenu =
        document.getElementById("botaoMenu");

    const menu =
        document.getElementById("menu");

    const linksMenu =
        document.querySelectorAll(".menu a");

    const themeToggle =
        document.getElementById("themeToggle");


    /* ========================================
       MODO CLARO / ESCURO
    ======================================== */

    const temaSalvo =
        localStorage.getItem("mindmovie-theme");


    if (temaSalvo === "light") {

        document.body.classList.add("light");

    } else {

        document.body.classList.remove("light");

    }


    if (themeToggle) {

        themeToggle.addEventListener(
            "click",
            function () {

                document.body.classList.toggle(
                    "light"
                );


                const modoClaro =
                    document.body.classList.contains(
                        "light"
                    );


                if (modoClaro) {

                    localStorage.setItem(
                        "mindmovie-theme",
                        "light"
                    );

                } else {

                    localStorage.setItem(
                        "mindmovie-theme",
                        "dark"
                    );

                }

            }
        );

    }


    /* ========================================
       ABRIR / FECHAR MENU MOBILE
    ======================================== */

    if (botaoMenu && menu) {

        botaoMenu.addEventListener(
            "click",
            function () {

                const menuEstaAberto =
                    menu.classList.toggle(
                        "aberto"
                    );


                botaoMenu.classList.toggle(
                    "ativo"
                );


                botaoMenu.setAttribute(
                    "aria-expanded",
                    menuEstaAberto
                        ? "true"
                        : "false"
                );


                document.body.style.overflow =
                    menuEstaAberto
                        ? "hidden"
                        : "";

            }
        );

    }


    /* ========================================
       FECHAR MENU AO CLICAR EM LINK
    ======================================== */

    linksMenu.forEach(function (link) {

        link.addEventListener(
            "click",
            function () {

                menu.classList.remove(
                    "aberto"
                );


                botaoMenu.classList.remove(
                    "ativo"
                );


                botaoMenu.setAttribute(
                    "aria-expanded",
                    "false"
                );


                document.body.style.overflow =
                    "";

            }
        );

    });


    /* ========================================
       IDENTIFICAR PÁGINA ATUAL
    ======================================== */

    let paginaAtual =
        window.location.pathname
            .split("/")
            .pop();


    if (paginaAtual === "") {

        paginaAtual =
            "componentes-e-valor.html";

    }


    linksMenu.forEach(function (link) {

        const paginaDoLink =
            link
                .getAttribute("href")
                .split("/")
                .pop();


        if (paginaDoLink === paginaAtual) {

            link.classList.add(
                "ativo"
            );

        } else {

            link.classList.remove(
                "ativo"
            );

        }

    });

});