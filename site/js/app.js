(function () {
  "use strict";

  var root = document.documentElement;
  var THEME_KEY = "mindmovie-theme";

  /* ---------------- Tema claro/escuro ---------------- */
  function applyTheme(theme) {
    root.setAttribute("data-theme", theme);
    try { localStorage.setItem(THEME_KEY, theme); } catch (e) { /* armazenamento indisponível */ }
  }

  function initTheme() {
    var saved = null;
    try { saved = localStorage.getItem(THEME_KEY); } catch (e) { /* ignorar */ }
    // Padrão: escuro (segue o design original), a menos que o usuário já tenha escolhido antes.
    applyTheme(saved === "light" || saved === "dark" ? saved : "dark");
  }

  var themeToggle = document.getElementById("themeToggle");
  if (themeToggle) {
    themeToggle.addEventListener("click", function () {
      var current = root.getAttribute("data-theme") === "light" ? "dark" : "light";
      applyTheme(current);
    });
  }

  initTheme();

  /* ---------------- Navegação entre seções (SPA simples) ---------------- */
  var navLinks = document.querySelectorAll("[data-nav-link]");
  var pages = document.querySelectorAll("[data-page]");
  var mobileMenu = document.getElementById("mobileMenu");
  var navToggle = document.getElementById("navToggle");

  function setActivePage(targetId, opts) {
    opts = opts || {};
    var found = false;

    pages.forEach(function (page) {
      var match = page.id === targetId;
      page.classList.toggle("is-active", match);
      if (match) found = true;
    });

    if (!found) return false;

    navLinks.forEach(function (link) {
      link.classList.toggle("is-active", link.getAttribute("data-target") === targetId);
    });

    if (opts.scroll !== false) {
      window.scrollTo({ top: 0, behavior: "auto" });
    }

    if (opts.updateHash !== false && history.replaceState) {
      history.replaceState(null, "", "#" + targetId);
    }

    return true;
  }

  navLinks.forEach(function (link) {
    link.addEventListener("click", function (evt) {
      var target = link.getAttribute("data-target");
      if (!target) return;
      evt.preventDefault();
      setActivePage(target);
      closeMobileMenu();
    });
  });

  // Abre a página correspondente ao hash da URL, se houver.
  (function initFromHash() {
    var hash = window.location.hash.replace("#", "");
    if (hash && document.getElementById(hash)) {
      setActivePage(hash, { updateHash: false });
    }
  })();

  window.addEventListener("popstate", function () {
    var hash = window.location.hash.replace("#", "");
    if (hash && document.getElementById(hash)) {
      setActivePage(hash, { updateHash: false });
    }
  });

  /* ---------------- Menu mobile ---------------- */
  function closeMobileMenu() {
    if (!mobileMenu || !navToggle) return;
    mobileMenu.classList.remove("is-open");
    navToggle.setAttribute("aria-expanded", "false");
  }

  function toggleMobileMenu() {
    if (!mobileMenu || !navToggle) return;
    var isOpen = mobileMenu.classList.toggle("is-open");
    navToggle.setAttribute("aria-expanded", String(isOpen));
  }

  if (navToggle) {
    navToggle.addEventListener("click", toggleMobileMenu);
  }

  document.addEventListener("keydown", function (evt) {
    if (evt.key === "Escape") closeMobileMenu();
  });

  window.addEventListener("resize", function () {
    if (window.innerWidth >= 860) closeMobileMenu();
  });

  /* ---------------- Busca simples no conteúdo ---------------- */
  var searchBox = document.getElementById("searchBox");
  var searchToggle = document.getElementById("searchToggle");
  var searchInput = document.getElementById("searchInput");

  function openSearch() {
    if (!searchBox || !searchInput || !searchToggle) return;
    searchBox.classList.add("is-open");
    searchToggle.setAttribute("aria-expanded", "true");
    searchInput.focus();
  }

  function closeSearch() {
    if (!searchBox || !searchToggle) return;
    searchBox.classList.remove("is-open");
    searchToggle.setAttribute("aria-expanded", "false");
  }

  if (searchToggle) {
    searchToggle.addEventListener("click", function () {
      if (searchBox.classList.contains("is-open")) {
        closeSearch();
      } else {
        openSearch();
      }
    });
  }

  if (searchInput) {
    searchInput.addEventListener("keydown", function (evt) {
      if (evt.key === "Escape") {
        searchInput.value = "";
        closeSearch();
        return;
      }
      if (evt.key !== "Enter") return;

      var query = searchInput.value.trim().toLowerCase();
      if (!query) return;

      var match = null;
      pages.forEach(function (page) {
        if (match) return;
        if (page.textContent.toLowerCase().indexOf(query) !== -1) {
          match = page.id;
        }
      });

      if (match) {
        setActivePage(match);
        closeMobileMenu();
      }
    });
  }

  document.addEventListener("click", function (evt) {
    if (!searchBox || !searchBox.classList.contains("is-open")) return;
    if (!searchBox.contains(evt.target)) closeSearch();
  });

  /* ---------------- Formulário de contato (sem backend) ---------------- */
  var contactForm = document.getElementById("contactForm");
  var contactNote = document.getElementById("contactNote");
  var defaultNoteText = contactNote ? contactNote.textContent : "";

  if (contactForm) {
    contactForm.addEventListener("submit", function (evt) {
      evt.preventDefault();
      if (!contactForm.checkValidity()) {
        contactForm.reportValidity();
        return;
      }

      if (contactNote) {
        contactNote.textContent = "Mensagem pronta para envio — configure um destino de e-mail para ativar o recebimento.";
        contactNote.classList.add("is-success");
      }

      contactForm.reset();

      if (contactNote) {
        window.setTimeout(function () {
          contactNote.textContent = defaultNoteText;
          contactNote.classList.remove("is-success");
        }, 4000);
      }
    });
  }
})();
