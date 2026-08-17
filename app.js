const deck = document.querySelector('[data-deck]');
const slides = [...document.querySelectorAll('[data-slide]')];
const timelineButtons = [...document.querySelectorAll('[data-go]')];
const slideCount = document.querySelector('[data-slide-count]');
const currentTitle = document.querySelector('[data-current-title]');
const previousButton = document.querySelector('[data-prev]');
const nextButton = document.querySelector('[data-next]');
const overviewButton = document.querySelector('[data-overview-toggle]');
const fullscreenButton = document.querySelector('[data-fullscreen]');
const softwareDetailPanel = document.querySelector('[data-software-detail-panel]');
const softwareDetailClose = document.querySelector('[data-software-detail-close]');
let activeIndex = 0;
let overviewOpen = false;
let lastSoftwareDetailTrigger = null;

function closeSoftwareDetail(restoreFocus = true) {
  if (!softwareDetailPanel || softwareDetailPanel.hidden) return;
  softwareDetailPanel.hidden = true;
  if (restoreFocus && lastSoftwareDetailTrigger) lastSoftwareDetailTrigger.focus();
}

function openSoftwareDetail(trigger) {
  if (!softwareDetailPanel) return;
  lastSoftwareDetailTrigger = trigger;
  const fields = {
    core: trigger.dataset.detailCore,
    title: trigger.dataset.detailTitle,
    body: trigger.dataset.detailBody,
    input: trigger.dataset.detailInput,
    output: trigger.dataset.detailOutput,
    code: trigger.dataset.detailCode,
  };
  Object.entries(fields).forEach(([name, value]) => {
    const target = softwareDetailPanel.querySelector(`[data-software-detail-${name}]`);
    if (target) target.textContent = value || '—';
  });
  softwareDetailPanel.hidden = false;
  softwareDetailClose?.focus();
}

function showSlide(index) {
  closeSoftwareDetail(false);
  activeIndex = Math.max(0, Math.min(slides.length - 1, index));
  slides.forEach((slide, slideIndex) => {
    const active = slideIndex === activeIndex;
    slide.classList.toggle('active', active);
    slide.setAttribute('aria-hidden', String(!active));
  });
  timelineButtons.forEach((button, buttonIndex) => {
    button.classList.toggle('active', buttonIndex === activeIndex);
    button.classList.toggle('visited', buttonIndex < activeIndex);
    button.setAttribute('aria-current', buttonIndex === activeIndex ? 'step' : 'false');
  });
  slideCount.textContent = `${String(activeIndex + 1).padStart(2, '0')} / ${String(slides.length).padStart(2, '0')}`;
  currentTitle.textContent = slides[activeIndex].dataset.title;
  previousButton.disabled = activeIndex === 0;
  nextButton.disabled = activeIndex === slides.length - 1;
  deck.style.setProperty('--deck-progress', `${((activeIndex + 1) / slides.length) * 100}%`);
  document.title = `${slides[activeIndex].dataset.title} / 光伏板智能巡检系统`;
}

function setOverview(open) {
  overviewOpen = open;
  document.body.classList.toggle('overview-open', open);
  overviewButton.setAttribute('aria-label', open ? '关闭总览' : '打开总览');
  if (!open) showSlide(activeIndex);
}

previousButton.addEventListener('click', () => showSlide(activeIndex - 1));
nextButton.addEventListener('click', () => showSlide(activeIndex + 1));
timelineButtons.forEach((button) => button.addEventListener('click', () => {
  showSlide(Number(button.dataset.go));
  if (overviewOpen) setOverview(false);
}));
overviewButton.addEventListener('click', () => setOverview(!overviewOpen));
slides.forEach((slide, index) => slide.addEventListener('click', () => {
  if (!overviewOpen) return;
  activeIndex = index;
  setOverview(false);
}));

document.addEventListener('keydown', (event) => {
  if (event.target.matches('[data-flow-detail]') && (event.key === ' ' || event.key === 'Enter')) return;
  if (event.target.matches('button') && (event.key === ' ' || event.key === 'Enter')) return;
  if (['ArrowRight', 'ArrowDown', 'PageDown', ' '].includes(event.key)) {
    event.preventDefault();
    showSlide(activeIndex + 1);
  }
  if (['ArrowLeft', 'ArrowUp', 'PageUp'].includes(event.key)) {
    event.preventDefault();
    showSlide(activeIndex - 1);
  }
  if (event.key === 'Home') showSlide(0);
  if (event.key === 'End') showSlide(slides.length - 1);
  if (event.key.toLowerCase() === 'o') setOverview(!overviewOpen);
  if (event.key.toLowerCase() === 'f') fullscreenButton.click();
  if (event.key === 'Escape' && softwareDetailPanel && !softwareDetailPanel.hidden) closeSoftwareDetail();
  else if (event.key === 'Escape' && overviewOpen) setOverview(false);
});

document.querySelectorAll('[data-flow-detail]').forEach((node) => {
  node.addEventListener('click', () => openSoftwareDetail(node));
  node.addEventListener('keydown', (event) => {
    if (event.key !== 'Enter' && event.key !== ' ') return;
    event.preventDefault();
    openSoftwareDetail(node);
  });
});
softwareDetailClose?.addEventListener('click', () => closeSoftwareDetail());
softwareDetailPanel?.addEventListener('click', (event) => {
  if (event.target === softwareDetailPanel) closeSoftwareDetail();
});

let pointerStartX = null;
deck.addEventListener('pointerdown', (event) => {
  if (event.target.closest('button, .model-viewer')) return;
  pointerStartX = event.clientX;
});
deck.addEventListener('pointerup', (event) => {
  if (pointerStartX === null) return;
  const distance = event.clientX - pointerStartX;
  pointerStartX = null;
  if (Math.abs(distance) < 70) return;
  showSlide(activeIndex + (distance < 0 ? 1 : -1));
});

fullscreenButton.addEventListener('click', async () => {
  try {
    if (!document.fullscreenElement) await document.documentElement.requestFullscreen();
    else await document.exitFullscreen();
  } catch (error) {
    console.warn('Fullscreen is unavailable:', error);
  }
});

const timerOutput = document.querySelector('[data-timer-output]');
const timerToggle = document.querySelector('[data-timer-toggle]');
const timerReset = document.querySelector('[data-timer-reset]');
let timerRunning = false;
let timerStartedAt = 0;
let accumulatedMs = 0;
let timerFrame = 0;

function formatTime(milliseconds) {
  const seconds = Math.floor(milliseconds / 1000);
  return `${String(Math.floor(seconds / 60)).padStart(2, '0')}:${String(seconds % 60).padStart(2, '0')}`;
}

function updateTimer() {
  const elapsed = accumulatedMs + (timerRunning ? performance.now() - timerStartedAt : 0);
  timerOutput.textContent = formatTime(elapsed);
  timerOutput.classList.toggle('warning', elapsed >= 13 * 60 * 1000);
  timerOutput.classList.toggle('overtime', elapsed >= 15 * 60 * 1000);
  if (timerRunning) timerFrame = requestAnimationFrame(updateTimer);
}

timerToggle.addEventListener('click', () => {
  timerRunning = !timerRunning;
  if (timerRunning) {
    timerStartedAt = performance.now();
    timerToggle.textContent = 'Ⅱ';
    timerToggle.setAttribute('aria-label', '暂停计时');
    updateTimer();
  } else {
    accumulatedMs += performance.now() - timerStartedAt;
    cancelAnimationFrame(timerFrame);
    timerToggle.textContent = '▶';
    timerToggle.setAttribute('aria-label', '继续计时');
    updateTimer();
  }
});

timerReset.addEventListener('click', () => {
  timerRunning = false;
  accumulatedMs = 0;
  cancelAnimationFrame(timerFrame);
  timerToggle.textContent = '▶';
  timerToggle.setAttribute('aria-label', '开始计时');
  updateTimer();
});

showSlide(0);
updateTimer();
