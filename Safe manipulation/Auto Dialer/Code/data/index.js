const tabButtons = document.querySelectorAll('.tab-button');
const tabContents = document.querySelectorAll('.tab-content');

function switchTab(targetTabId) {
    tabButtons.forEach(button => {
        button.classList.remove('active');
    });

    tabContents.forEach(content => {
        content.classList.remove('active');
    });

    const activeButton = document.querySelector(`[data-tab="${targetTabId}"]`);
    if (activeButton) {
        activeButton.classList.add('active');
    }

    const activeContent = document.getElementById(targetTabId);
    if (activeContent) {
        activeContent.classList.add('active');
        activeContent.scrollTop = 0;
    }
}

document.addEventListener('DOMContentLoaded', function() {
    tabButtons.forEach(button => {
        button.addEventListener('click', function() {
            const targetTab = this.getAttribute('data-tab');
            switchTab(targetTab);
        });
    });
    
    document.addEventListener('keydown', function(e) {
        if (!e.target.classList.contains('tab-button')) {
            return;
        }
        
        const currentIndex = Array.from(tabButtons).indexOf(e.target);
        let newIndex;
        
        switch(e.key) {
            case 'ArrowLeft':
                e.preventDefault();
                newIndex = currentIndex > 0 ? currentIndex - 1 : tabButtons.length - 1;
                tabButtons[newIndex].focus();
                switchTab(tabButtons[newIndex].getAttribute('data-tab'));
                break;
                
            case 'ArrowRight':
                e.preventDefault();
                newIndex = currentIndex < tabButtons.length - 1 ? currentIndex + 1 : 0;
                tabButtons[newIndex].focus();
                switchTab(tabButtons[newIndex].getAttribute('data-tab'));
                break;
                
            case 'Home':
                e.preventDefault();
                tabButtons[0].focus();
                switchTab(tabButtons[0].getAttribute('data-tab'));
                break;
                
            case 'End':
                e.preventDefault();
                tabButtons[tabButtons.length - 1].focus();
                switchTab(tabButtons[tabButtons.length - 1].getAttribute('data-tab'));
                break;
        }
    });
    
    if (tabButtons.length > 0 && tabContents.length > 0) {
        const firstTab = tabButtons[0].getAttribute('data-tab');
        switchTab(firstTab);
    }
});

function expandExclusions(str) {
  const result = [];
  if (!str.trim()) return result;
  str.split(',').forEach(token => {
    token = token.trim();
    const dashIdx = token.indexOf('-');
    if (dashIdx > 0) {
      const a = parseInt(token.slice(0, dashIdx));
      const b = parseInt(token.slice(dashIdx + 1));
      if (b >= a) {
        for (let n = a; n <= b; n++) result.push(n);
      } else {
        for (let n = a; n <= 99; n++) result.push(n);
        for (let n = 0; n <= b; n++) result.push(n);
      }
    } else {
      result.push(parseInt(token));
    }
  });
  return result;
}

const output = document.getElementById("output");
function appendLog(message) {
  const line = document.createElement("p");
  line.textContent = message;
  output.appendChild(line);
  const autoScroll = document.getElementById("autoscroll-checkbox");
  if (!autoScroll || autoScroll.checked) {
    output.scrollTop = output.scrollHeight;
  }
}

const evtSource = new EventSource("/events");
evtSource.addEventListener("log", e => appendLog(e.data));
evtSource.addEventListener("status", e => updateLiveView(e.data));
evtSource.onopen = () => appendLog("Connected.");
evtSource.onerror = function() {
  if (evtSource.readyState === EventSource.CLOSED) {
    appendLog("Disconnected.");
  }
};

let elapsedTimerInterval = null;
let elapsedTimerRunning = false;
let elapsedStartTime = 0;

function formatElapsed(ms) {
  const totalSec = Math.floor(ms / 1000);
  const h = Math.floor(totalSec / 3600);
  const m = Math.floor((totalSec % 3600) / 60);
  const s = totalSec % 60;
  if (h > 0) {
    return h + "h " + String(m).padStart(2, "0") + "m " + String(s).padStart(2, "0") + "s";
  } else if (m > 0) {
    return m + "m " + String(s).padStart(2, "0") + "s";
  } else {
    return s + "s";
  }
}

function startElapsedTimer() {
  elapsedStartTime = Date.now();
  elapsedTimerRunning = true;
  elapsedTimerInterval = setInterval(() => {
    document.getElementById("elapsed-label").textContent =
      "Elapsed Time: " + formatElapsed(Date.now() - elapsedStartTime);
  }, 1000);
}

function stopElapsedTimer() {
  clearInterval(elapsedTimerInterval);
  elapsedTimerInterval = null;
  elapsedTimerRunning = false;
}

function updateLiveView(jsonStr) {
  let data;
  try { data = JSON.parse(jsonStr); } catch(e) { return; }

  const statusText = document.getElementById("status-text");
  statusText.textContent = data.status;
  statusText.className = "";
  if (data.status === "Waiting") {
    statusText.className = "status-waiting";
  } else if (data.status === "EMERGENCY STOP") {
    statusText.className = "status-emergency";
  } else {
    statusText.className = "status-trying";
  }

  const header = document.getElementById("live-wheels-header");
  const values = document.getElementById("live-wheels-values");

  if (data.wheels && data.wheels.length > 0) {
    header.innerHTML = "";
    values.innerHTML = "";
    data.wheels.forEach((num, i) => {
      const hCell = document.createElement("div");
      hCell.className = "live-wheel-cell";
      hCell.textContent = "Wheel " + (i + 1);
      header.appendChild(hCell);

      if (i < data.wheels.length - 1) {
        const sep = document.createElement("div");
        sep.className = "live-wheel-sep";
        sep.textContent = "-";
        header.appendChild(sep);
      }

      const vCell = document.createElement("div");
      vCell.className = "live-wheel-cell live-wheel-num";
      vCell.textContent = num;
      values.appendChild(vCell);

      if (i < data.wheels.length - 1) {
        const sep2 = document.createElement("div");
        sep2.className = "live-wheel-sep";
        sep2.textContent = "";
        values.appendChild(sep2);
      }
    });
  }

  const current = data.current || 0;
  const total = data.total || 0;
  const progressFill = document.getElementById("progress-bar-fill");
  const progressLabel = document.getElementById("progress-label");

  if (total > 0) {
    const pct = Math.min(100, (current / total) * 100);
    progressFill.style.width = pct + "%";
    progressLabel.textContent = current + " / " + total;
  }

  const elapsedLabel = document.getElementById("elapsed-label");
  if (data.status === "Waiting") {
    stopElapsedTimer();
  } else if (data.status === "EMERGENCY STOP") {
    stopElapsedTimer();
  } else if (!elapsedTimerRunning) {
    startElapsedTimer();
  }
}

document.getElementById("cur-num-input").addEventListener("keydown", function (e) {
    if (e.key === "Enter") {
      e.preventDefault();
      document.getElementById("center-button").click();
    }
});

document.getElementById("quick-start-button").addEventListener("click", function () {
  const rcp = document.getElementById("rcp-input").value;
  const lcp = document.getElementById("lcp-input").value;
  const everyX = document.getElementById("everyX").value;
  const avoidWithin = document.getElementById("avoid-range").value;
  const openRot = document.getElementById("open-rot").value;
  const openDistance = document.getElementById("open-distance").value;
  const rotConversion = document.getElementById("rotConversion").value;
  const dropCheck = document.getElementById("drop-check").value;
  const speed = document.getElementById("speed").value;
  const w1start = document.getElementById("w1start").value;
  
  if (!rcp || !lcp) {
    alert("Please enter both contact points before starting!");
    return;
  }
  const payload = {
    wheels: [
      { wheel: 1, openRot: 'L', exclusions: [] },
      { wheel: 2, openRot: 'R', exclusions: [] },
      { wheel: 3, openRot: 'L', exclusions: [] }
    ],
    rcp: parseFloat(rcp),
    lcp: parseFloat(lcp),
    everyX: parseFloat(everyX),
    avoidWithin: parseFloat(avoidWithin),
    openRot: openRot,
    openDistance: parseFloat(openDistance),
    rotConversion: parseFloat(rotConversion),
    dropCheck: dropCheck,
    speed: speed,
    w1start: w1start
  };
  fetch('/start', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload)
  })
    .then(response => response.text())
    .then(text => {
      console.log("Start response:", text);
      switchTab('view');
    })
    .catch(error => console.error('Error:', error));
});

// Handles clicking the start button
document.getElementById("start-button").addEventListener("click", function () {
  const wheelElements = document.querySelectorAll(".wheel");
  if (wheelElements.length === 0) {
    alert("Please add at least one wheel before starting!");
    return;
  }
  const wheels = [];
  wheelElements.forEach((wheelEl, index) => {
    const labelHTML = wheelEl.querySelector(".wheel-label").innerHTML;
    const openMatch = labelHTML.match(/Open:\s*(Left|Right)/);
    const openRot = openMatch ? (openMatch[1] === 'Left' ? 'L' : 'R') : 'L';
    const exclusions = wheelEl.dataset.exclusions || "";
    wheels.push({
      wheel: index + 1,
      openRot: openRot,
      exclusions: expandExclusions(exclusions)
    });
  });
  const rcp = document.getElementById("rcp-input").value;
  const lcp = document.getElementById("lcp-input").value;
  const everyX = document.getElementById("everyX").value;
  const avoidWithin = document.getElementById("avoid-range").value;
  const openRot = document.getElementById("open-rot").value;
  const openDistance = document.getElementById("open-distance").value;
  const rotConversion = document.getElementById("rotConversion").value;
  const dropCheck = document.getElementById("drop-check").value;
  const speed = document.getElementById("speed").value;
  if (!rcp || !lcp) {
    alert("Please enter both contact points before starting!");
    return;
  }
  const payload = {
    wheels: wheels,
    rcp: parseFloat(rcp),
    lcp: parseFloat(lcp),
    everyX: parseFloat(everyX),
    avoidWithin: parseFloat(avoidWithin),
    openRot: openRot,
    openDistance: parseFloat(openDistance),
    rotConversion: parseFloat(rotConversion),
    dropCheck: dropCheck,
    speed: speed
  };
  fetch('/start', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload)
  })
    .then(response => response.text())
    .then(text => {
      console.log("Start response:", text);
      switchTab('view');
    })
    .catch(error => console.error('Error:', error));
});

document.getElementById("stop-button").addEventListener("click", function () {
  fetch('/stop')
      .then(response => response.text())
      .then(() => {})
      .catch(error => console.error('Error:', error));
});

document.getElementById("spin-button").addEventListener("click", function () {
  fetch('/spin')
      .then(response => response.text())
      .then(() => {})
      .catch(error => console.error('Error:', error));
});

document.getElementById("center-button").addEventListener("click", function () {
    const center = document.getElementById('cur-num-input').value;
    if (center.includes("e")) {
      alert("Only enter numbers and, if needed, a period for decimals!");
      return;
    }
    fetch('/centerdial?center=' + encodeURIComponent(center))
        .then(response => response.text())
        .then(() => {
            document.getElementById('cur-num-input').value = '';
        })
        .catch(error => console.error('Error:', error));
});

document.getElementById("add-wheel-button").addEventListener("click", function () {
  addWheel();
});

let wheelCount = 1;
let removedWheelIndex = [];

function addWheel() {
  const btn = document.createElement("button");
  const box = document.getElementById("wheels");
  const label = document.createElement("div");
  const newItem = document.createElement("div");
  const openRot = document.getElementById('wheel-open-rot').value;
  const openRotText = openRot === 'L' ? 'Left' : 'Right';

  // Capture raw exclusions string before any formatting
  const rawExclusions = document.getElementById('exclusions-input').value.replace(/,+$/, "").replace(/\s+/g, "");

  let labelString = rawExclusions;
  const exclusionsMatches = labelString.match(/\d+/g) || [];
  const exclusionsValid = exclusionsMatches.every(numStr => parseInt(numStr, 10) <= 100);
  if ((!/^[0-9\-,\s]*$/.test(labelString) || !exclusionsValid || /\d+\s+\d+/.test(labelString)) && labelString.trim() !== "") {
    alert("Only enter comma separated whole numbers not greater than 100, commas, and dashes!");
    return;
  }

  document.getElementById('exclusions-input').value = labelString;

  // Format labelString for display only
  let displayString = labelString;
  if (displayString.length > 16) {
    const splitIndex = displayString.lastIndexOf(",", 16);
    if (splitIndex !== -1) {
      displayString =
        displayString.slice(0, splitIndex + 1) + "<br>" +
        displayString.slice(splitIndex + 1);
    }
  } else if (displayString.trim() === "") {
    displayString = "<br><br>";
  } else {
    displayString += "<br><br>";
  }

  btn.className = "dynamic-btn";
  label.className = "wheel-label";
  newItem.className = "wheel";
  newItem.dataset.exclusions = rawExclusions;  // Store raw exclusions on the element
  label.innerHTML = `<u>Wheel ${wheelCount++}</u><br>Open: ${openRotText}<br>Exclusions<br>` + displayString;
  btn.textContent = "Remove";
  newItem.appendChild(label);
  newItem.appendChild(btn);

  btn.onclick = function () {
    const labelNode = this.parentNode.querySelector('.wheel-label');
    const match = labelNode.innerHTML.match(/\d+/);
    const firstInt = match ? parseInt(match[0], 10) : null;
    removedWheelIndex.push(firstInt - 1);
    removedWheelIndex.sort((a, b) => a - b);
    this.parentElement.remove();
    wheelCount--;
  };

  const items = box.querySelectorAll(".wheel");
  let inserted = false;
  if (removedWheelIndex.length !== 0 && items.length > 0) {
    for (let index = 0; index < items.length; index++) {
      const item = items[index];
      const oldLabel = item.querySelector(".wheel-label");
      const wheelNumElem = oldLabel.querySelector("u");
      if (wheelNumElem && wheelNumElem.textContent !== `Wheel ${index + 1}` && !inserted) {
        const insertPos = removedWheelIndex[0];
        const referenceNode = document.getElementById("wheels").children[insertPos];
        label.innerHTML = `<u>Wheel ${removedWheelIndex[0] + 1}</u><br>Open: ${openRotText}<br>Exclusions<br>` + displayString;
        newItem.dataset.exclusions = rawExclusions;  // Also set on re-inserted wheels
        if (referenceNode) {
          box.insertBefore(newItem, referenceNode);
        } else {
          box.appendChild(newItem);
        }
        removedWheelIndex.shift();
        inserted = true;
        break;
      }
    }
  }

  if (!inserted) {
    box.appendChild(newItem);
    if (removedWheelIndex.length > 0) {
      removedWheelIndex.shift();
    }
  }

  document.getElementById("exclusions-input").value = "";
}