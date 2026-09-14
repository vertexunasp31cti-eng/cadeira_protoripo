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

    const abrirVideo =
        document.getElementById("abrirVideo");

    const fecharVideo =
        document.getElementById("fecharVideo");

    const fundoModal =
        document.getElementById("fundoModal");

    const modalVideo =
        document.getElementById("modalVideo");

    const videoDemonstracao =
        document.getElementById("videoDemonstracao");


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
       FECHAR MENU AO CLICAR NO LINK
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
       PÁGINA ATUAL
    ======================================== */

    let paginaAtual =
        window.location.pathname
            .split("/")
            .pop();


    if (paginaAtual === "") {

        paginaAtual = "index.html";

    }


    linksMenu.forEach(function (link) {

        const paginaDoLink =
            link
                .getAttribute("href")
                .split("/")
                .pop();


        if (paginaDoLink === paginaAtual) {

            link.classList.add("ativo");

        } else {

            link.classList.remove("ativo");

        }

    });


    /* ========================================
       ABRIR VÍDEO
    ======================================== */

    function mostrarVideo() {

        if (!modalVideo) {
            return;
        }


        modalVideo.classList.add(
            "aberto"
        );


        modalVideo.setAttribute(
            "aria-hidden",
            "false"
        );


        document.body.style.overflow =
            "hidden";


        if (videoDemonstracao) {

            videoDemonstracao
                .play()
                .catch(function () {

                    console.log(
                        "O vídeo aguardará o usuário apertar o botão de reprodução."
                    );

                });

        }

    }


    /* ========================================
       FECHAR VÍDEO
    ======================================== */

    function esconderVideo() {

        if (!modalVideo) {
            return;
        }


        modalVideo.classList.remove(
            "aberto"
        );


        modalVideo.setAttribute(
            "aria-hidden",
            "true"
        );


        if (videoDemonstracao) {

            videoDemonstracao.pause();

            videoDemonstracao.currentTime = 0;

        }


        document.body.style.overflow = "";

    }


    /* ========================================
       EVENTOS DO VÍDEO
    ======================================== */

    if (abrirVideo) {

        abrirVideo.addEventListener(
            "click",
            mostrarVideo
        );

    }


    if (fecharVideo) {

        fecharVideo.addEventListener(
            "click",
            esconderVideo
        );

    }


    if (fundoModal) {

        fundoModal.addEventListener(
            "click",
            esconderVideo
        );

    }


    /* ========================================
       FECHAR COM ESC
    ======================================== */

    document.addEventListener(
        "keydown",
        function (evento) {

            if (
                evento.key === "Escape" &&
                modalVideo &&
                modalVideo.classList.contains(
                    "aberto"
                )
            ) {

                esconderVideo();

            }

        }
    );

});