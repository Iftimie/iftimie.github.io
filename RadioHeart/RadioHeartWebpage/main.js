(() => {
  const year = document.getElementById("year");
  if (year) year.textContent = String(new Date().getFullYear());

  // Subtle section reveals
  const revealEls = Array.from(document.querySelectorAll("[data-reveal]"));
  if (revealEls.length) {
    const reduce = window.matchMedia("(prefers-reduced-motion: reduce)").matches;
    if (reduce || !("IntersectionObserver" in window)) {
      revealEls.forEach((el) => el.classList.add("is-visible"));
    } else {
      const io = new IntersectionObserver(
        (entries) => {
          for (const entry of entries) {
            if (entry.isIntersecting) {
              entry.target.classList.add("is-visible");
              io.unobserve(entry.target);
            }
          }
        },
        { threshold: 0.12, rootMargin: "0px 0px -10% 0px" }
      );
      revealEls.forEach((el) => io.observe(el));
    }
  }

  // Story + ritual beats reveal
  const beatEls = Array.from(document.querySelectorAll("[data-beat]"));
  if (beatEls.length) {
    const reduce = window.matchMedia("(prefers-reduced-motion: reduce)").matches;
    if (reduce || !("IntersectionObserver" in window)) {
      beatEls.forEach((el) => el.classList.add("in-view"));
    } else {
      const beatIO = new IntersectionObserver(
        (entries) => {
          for (const entry of entries) {
            if (entry.isIntersecting) {
              entry.target.classList.add("in-view");
              beatIO.unobserve(entry.target);
            }
          }
        },
        { threshold: 0.35 }
      );
      beatEls.forEach((el) => beatIO.observe(el));
    }
  }

  // Story progress indicator (only during story)
  const story = document.querySelector(".story");
  if (story) {
    const reduce = window.matchMedia("(prefers-reduced-motion: reduce)").matches;
    const progress = story.querySelector(".story-progress");

    const updateProgress = () => {
      const rect = story.getBoundingClientRect();
      const viewportH = window.innerHeight || 1;
      const scrollable = rect.height - viewportH;
      const raw = scrollable <= 0 ? (rect.top < 0 ? 1 : 0) : (-rect.top / scrollable);
      const clamped = Math.max(0, Math.min(1, raw));
      story.style.setProperty("--story-progress", String(Math.round(clamped * 1000) / 10));
    };

    const setActive = (active) => {
      story.classList.toggle("is-active", active);
    };

    if (progress) {
      const storyIO = new IntersectionObserver(
        (entries) => {
          const entry = entries[0];
          setActive(Boolean(entry && entry.isIntersecting));
          if (!reduce && entry && entry.isIntersecting) updateProgress();
        },
        { threshold: 0.02 }
      );
      storyIO.observe(story);
    }

    if (!reduce) {
      const onScroll = () => updateProgress();
      window.addEventListener("scroll", onScroll, { passive: true });
      window.addEventListener("resize", onScroll);
      updateProgress();
    }
  }

  // Story anchor image swaps (group-driven)
  const anchorA = document.querySelector(".story-anchor-img--a");
  const anchorB = document.querySelector(".story-anchor-img--b");
  const groups = Array.from(document.querySelectorAll(".beat-group[data-anchor]"));
  if (story && anchorA && anchorB && groups.length) {
    const reduce = window.matchMedia("(prefers-reduced-motion: reduce)").matches;
    const mqMobile = window.matchMedia("(max-width: 900px)");
    const srcFor = (key) => {
      switch (String(key || "").trim()) {
        case "pair":
          return "assets/images/product-pair.png";
        case "her":
          return "assets/images/her.png";
        case "him":
          return "assets/images/him.png";
        case "desk":
          return "assets/images/noptiera.png";
        default:
          return "assets/images/product-pair.png";
      }
    };

    let activeKey = null;
    let activeEl = anchorA;
    let inactiveEl = anchorB;

    const swapTo = (key) => {
      if (!key || key === activeKey) return;
      activeKey = key;
      const nextSrc = srcFor(key);

      if (reduce) {
        activeEl.src = nextSrc;
        activeEl.classList.add("is-active");
        inactiveEl.classList.remove("is-active");
        return;
      }

      inactiveEl.src = nextSrc;
      inactiveEl.classList.add("is-active");
      activeEl.classList.remove("is-active");
      const tmp = activeEl;
      activeEl = inactiveEl;
      inactiveEl = tmp;
    };

    // Initialize from first group
    const initialKey = groups[0].getAttribute("data-anchor") || "pair";
    anchorA.src = srcFor(initialKey);
    swapTo(initialKey);

    let cleanup = () => {};

    const setupMobile = () => {
      // On small screens the sticky + short beats can make IO thresholds flaky.
      // Instead, choose the beat group that contains (or is nearest to) a viewport
      // focus point while scrolling.
      const focusY = () => (window.innerHeight || 1) * 0.38;
      let raf = 0;

      const updateFromScroll = () => {
        raf = 0;
        const fy = focusY();
        let best = null;
        let bestDist = Infinity;

        for (const g of groups) {
          const rect = g.getBoundingClientRect();
          const clampedY = Math.min(Math.max(fy, rect.top), rect.bottom);
          const dist = Math.abs(clampedY - fy);
          if (dist < bestDist) {
            bestDist = dist;
            best = g;
          }
        }

        const key = best ? best.getAttribute("data-anchor") : null;
        swapTo(key);
      };

      const onScroll = () => {
        if (raf) return;
        raf = window.requestAnimationFrame(updateFromScroll);
      };

      window.addEventListener("scroll", onScroll, { passive: true });
      window.addEventListener("resize", onScroll);
      onScroll();

      return () => {
        window.removeEventListener("scroll", onScroll);
        window.removeEventListener("resize", onScroll);
        if (raf) window.cancelAnimationFrame(raf);
      };
    };

    const setupDesktop = () => {
      const groupIO = new IntersectionObserver(
        (entries) => {
          const candidates = entries
            .filter((e) => e.isIntersecting)
            .sort((a, b) => b.intersectionRatio - a.intersectionRatio);
          if (!candidates.length) return;
          const key = candidates[0].target.getAttribute("data-anchor");
          swapTo(key);
        },
        { threshold: [0.35, 0.5, 0.65], rootMargin: "-10% 0px -55% 0px" }
      );
      groups.forEach((g) => groupIO.observe(g));

      return () => {
        groupIO.disconnect();
      };
    };

    const setup = () => {
      cleanup();
      cleanup = mqMobile.matches ? setupMobile() : setupDesktop();
    };

    setup();

    // If someone resizes (or uses device emulation), keep behavior consistent.
    if ("addEventListener" in mqMobile) {
      mqMobile.addEventListener("change", setup);
    } else {
      mqMobile.addListener(setup);
    }
  }
})();
