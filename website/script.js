document.addEventListener('DOMContentLoaded', () => {
  // Cursor glow effect tracking
  const glow = document.getElementById('cursor-glow');
  if (glow) {
    document.addEventListener('mousemove', (e) => {
      glow.style.setProperty('--mouse-x', `${e.clientX}px`);
      glow.style.setProperty('--mouse-y', `${e.clientY}px`);
    });
  }

  // Example Tab Switcher Logic
  window.switchExampleTab = function(lang) {
    const tabs = ['kr', 'ja', 'en'];
    tabs.forEach(t => {
      const btn = document.getElementById('tab-btn-' + t);
      const content = document.getElementById('tab-content-' + t);
      if (btn && content) {
        if (t === lang) {
          btn.classList.add('text-white', 'border-slate-200');
          btn.classList.remove('text-slate-400', 'border-transparent');
          content.classList.remove('hidden');
        } else {
          btn.classList.remove('text-white', 'border-slate-200');
          btn.classList.add('text-slate-400', 'border-transparent');
          content.classList.add('hidden');
        }
      }
    });
  };

  // ITN Example Tab Switcher Logic
  window.switchItnExampleTab = function(lang) {
    const tabs = ['kr', 'ja', 'en'];
    tabs.forEach(t => {
      const btn = document.getElementById('tab-itn-btn-' + t);
      const content = document.getElementById('tab-itn-content-' + t);
      if (btn && content) {
        if (t === lang) {
          btn.classList.add('text-white', 'border-slate-200');
          btn.classList.remove('text-slate-400', 'border-transparent');
          content.classList.remove('hidden');
        } else {
          btn.classList.remove('text-white', 'border-slate-200');
          btn.classList.add('text-slate-400', 'border-transparent');
          content.classList.add('hidden');
        }
      }
    });
  };

  // Neural Head Tab Switcher Logic
  window.switchHeadTab = function(lang) {
    const tabs = ['kr', 'ja', 'en'];
    tabs.forEach(t => {
      const btn = document.getElementById('tab-head-btn-' + t);
      const content = document.getElementById('tab-head-content-' + t);
      if (btn && content) {
        if (t === lang) {
          btn.classList.add('text-white', 'border-slate-200');
          btn.classList.remove('text-slate-400', 'border-transparent');
          content.classList.remove('hidden');
        } else {
          btn.classList.remove('text-white', 'border-slate-200');
          btn.classList.add('text-slate-400', 'border-transparent');
          content.classList.add('hidden');
        }
      }
    });
  };

  // 3-Language Toggle (EN / KO / JA)
  const btnEn = document.getElementById('btn-en');
  const btnKo = document.getElementById('btn-ko');
  const btnJa = document.getElementById('btn-ja');
  const translatableElements = document.querySelectorAll('[data-en][data-ko]');

  function setLanguage(lang) {
    [btnEn, btnKo, btnJa].forEach(btn => {
      if (btn) btn.classList.remove('active');
    });

    if (lang === 'en' && btnEn) btnEn.classList.add('active');
    if (lang === 'ko' && btnKo) btnKo.classList.add('active');
    if (lang === 'ja' && btnJa) btnJa.classList.add('active');

    translatableElements.forEach(el => {
      const text = el.getAttribute(`data-${lang}`);
      if (text) {
        el.innerHTML = text;
      }
    });

    // Synchronize TN Example Tab, ITN Example Tab & Neural Head Tab with Main Header Language Toggle
    const targetTab = (lang === 'ko') ? 'kr' : lang;
    window.switchExampleTab(targetTab);
    window.switchItnExampleTab(targetTab);
    window.switchHeadTab(targetTab);
  }

  if (btnEn) btnEn.addEventListener('click', () => setLanguage('en'));
  if (btnKo) btnKo.addEventListener('click', () => setLanguage('ko'));
  if (btnJa) btnJa.addEventListener('click', () => setLanguage('ja'));

  // High-precision ScrollSpy using IntersectionObserver
  const sections = document.querySelectorAll('section[id]');
  const navLinks = document.querySelectorAll('.nav a');

  function updateActiveNav(activeId) {
    navLinks.forEach(link => {
      if (link.getAttribute('href') === `#${activeId}`) {
        link.classList.add('active');
      } else {
        link.classList.remove('active');
      }
    });
  }

  const observerOptions = {
    root: null,
    rootMargin: '-20% 0px -50% 0px',
    threshold: 0.1
  };

  const observer = new IntersectionObserver((entries) => {
    entries.forEach(entry => {
      if (entry.isIntersecting) {
        updateActiveNav(entry.target.id);
      }
    });
  }, observerOptions);

  sections.forEach(section => {
    observer.observe(section);
  });

  // Explicitly handle nav click so menu updates instantly even if scroll distance is minimal (e.g. Git Repositories -> About Us)
  navLinks.forEach(link => {
    link.addEventListener('click', () => {
      const href = link.getAttribute('href');
      if (href && href.startsWith('#')) {
        const targetId = href.substring(1);
        updateActiveNav(targetId);
      }
    });
  });

  // Handle bottom of page scroll to guarantee active state on last nav item (About Us)
  window.addEventListener('scroll', () => {
    if ((window.innerHeight + window.scrollY) >= document.documentElement.scrollHeight - 40) {
      const lastSection = sections[sections.length - 1];
      if (lastSection) {
        updateActiveNav(lastSection.id);
      }
    }
  }, { passive: true });
});

