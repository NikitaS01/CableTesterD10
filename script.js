/**
 * ============================================================================
 * DIY Кабельный тестер NKS - Логика интерактивного сайта
 * ============================================================================
 */

// Флаг состояния поворота экрана (180 градусов)
let isRotated = false;

/**
 * Объект конфигураций состояний интерактивного симулятора OLED-дисплея.
 * Содержит данные для эмуляции режимов: OK, Short Circuit, Swap, Open Circuit, Slave Off.
 */
const simStates = {
  // Режим: Все жилы исправны (10 жил OK)
  ok: {
    role: "MASTER", 
    bat: "98% 4.1V", 
    batIcon: "[====|]", 
    alert: "",
    items: [
      { label: "1>1 OK", err: false }, { label: "6>6 OK", err: false },
      { label: "2>2 OK", err: false }, { label: "7>7 OK", err: false },
      { label: "3>3 OK", err: false }, { label: "8>8 OK", err: false },
      { label: "4>4 OK", err: false }, { label: "9>9 OK", err: false },
      { label: "5>5 OK", err: false }, { label: "10>10 OK", err: false }
    ]
  },
  // Режим: Короткое замыкание между жилами 5 и 6
  short: {
    role: "MASTER", 
    bat: "98% 4.1V", 
    batIcon: "[====|]", 
    alert: "KZ DETECTED ",
    items: [
      { label: "1>1 OK", err: false }, { label: "2>2 OK", err: false },
      { label: "3>3 OK", err: false }, { label: "4>4 OK", err: false },
      { label: "5>[!]", err: true },  { label: "6>[!]", err: true },
      { label: "7>7 OK", err: false }, { label: "8>8 OK", err: false },
      { label: "9>9 OK", err: false }, { label: "10>10 OK", err: false }
    ]
  },
  // Режим: Пересортица (перепутаны жилы 1 и 2)
  swap: {
    role: "MASTER", 
    bat: "95% 4.0V", 
    batIcon: "[====|]", 
    alert: "",
    items: [
      { label: "1>1 OK", err: false },  { label: "6>6 OK", err: false },
      { label: "2>2 OK", err: false },  { label: "7>4 ! ", err: true },
      { label: "3>3 OK", err: false }, { label: "8>8 OK", err: false },
      { label: "4>7 ! ", err: true }, { label: "9>9 OK", err: false },
      { label: "5>5 OK", err: false }, { label: "10>10 OK", err: false }
    ]
  },
  // Режим: Обрыв жилы #3
  open: {
    role: "MASTER", 
    bat: "92% 3.9V", 
    batIcon: "[===-|]", 
    alert: "",
    items: [
      { label: "1>1 OK", err: false }, { label: "6>6 OK", err: false },
      { label: "2>2 OK", err: false }, { label: "7>7 OK", err: false },
      { label: "3>-- X", err: true }, { label: "8>8 OK", err: false },
      { label: "4>4 OK", err: false }, { label: "9>9 OK", err: false },
      { label: "5>5 OK", err: false }, { label: "10>10 OK", err: false }
    ]
  },
  // Режим: Модуль Slave отключен
  slave_off: {
    role: "SLAVE", 
    bat: "100% 4.2V", 
    batIcon: "[=====]", 
    alert: "SLAVE POWER OFF OR DISCONNECTED",
    
  }
};

/**
 * Переключает режим работы симулятора и обновляет содержимое виртуального OLED-экрана.
 * @param {string} mode - Название режима ('ok', 'short', 'swap', 'open', 'slave_off')
 */
function setSimMode(mode) {
  const data = simStates[mode];
  if (!data) return;

  // Обновление верхнего бара статуса OLED
  document.getElementById('oledRole').innerText = data.role;
  document.getElementById('oledBat').innerText = data.bat;
  document.getElementById('oledBatIcon').innerText = data.batIcon;

  // Управление блоком тревожных сообщений
  const alertEl = document.getElementById('oledAlert');
  if (data.alert) {
    alertEl.style.display = "block";
    alertEl.innerText = data.alert;
  } else {
    alertEl.style.display = "none";
  }

  // Динамическая генерация сетки каналов (10 жил)
  const gridEl = document.getElementById('oledGrid');
  gridEl.innerHTML = "";
  data.items.forEach(item => {
    const div = document.createElement('div');
    div.className = "oled-item" + (item.err ? " err" : "");
    div.innerText = item.label;
    gridEl.appendChild(div);
  });

  // Обновление подсветки активной кнопки управления симулятором
  document.querySelectorAll('.sim-btn').forEach(btn => btn.classList.remove('active'));
  if (mode === 'ok') document.getElementById('btnOk')?.classList.add('active');
  if (mode === 'short') document.getElementById('btnShort')?.classList.add('active');
  if (mode === 'swap') document.getElementById('btnSwap')?.classList.add('active');
  if (mode === 'open') document.getElementById('btnOpen')?.classList.add('active');
  if (mode === 'slave_off') document.getElementById('btnSlave')?.classList.add('active');
}

/**
 * Поворачивает эмуляцию экрана OLED на 180 градусов (имитация физической кнопки устройства).
 */
function toggleRotateScreen() {
  isRotated = !isRotated;
  const oled = document.getElementById('oledScreen');
  if (oled) {
    oled.style.transform = isRotated ? 'rotate(180deg)' : 'rotate(0deg)';
  }
}

/**
 * Инициализация обработчиков событий при загрузке страницы.
 */
document.addEventListener('DOMContentLoaded', () => {
  // Установка начального режима симулятора (Короткое замыкание)
  setSimMode('short');

  // Управление мобильным меню (Гамбургер)
  const mobileBtn = document.getElementById('mobileMenuBtn');
  const navLinks = document.getElementById('navLinks');

  if (mobileBtn && navLinks) {
    mobileBtn.addEventListener('click', () => {
      navLinks.classList.toggle('active');
    });

    // Автоматическое закрытие мобильного меню при клике на любую ссылку
    document.querySelectorAll('.nav-item').forEach(item => {
      item.addEventListener('click', () => {
        navLinks.classList.remove('active');
      });
    });
  }
});
