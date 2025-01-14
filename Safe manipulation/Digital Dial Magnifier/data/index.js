const graph1 = new Chart(document.getElementById("graph1").getContext("2d"), {
  type: "scatter",
  data: {
    datasets: [
      {
        label: "RCP",
        data: [],
        borderColor: "rgb(255, 255, 255)",
        backgroundColor: "rgb(255, 255, 255)",
      }
    ],
  },
  options: {
    plugins: {
      legend: { display: true },
    }
  },
});

const graph2 = new Chart(document.getElementById("graph2").getContext("2d"), {
  type: "scatter",
  data: {
    datasets: [
      {
        label: "LCP",
        data: [],
        borderColor: "rgb(255, 255, 255)",
        backgroundColor: "rgb(255, 255, 255)",
      }
    ],
  },
  options: {
    plugins: {
      legend: { display: true },
    }
  },
});

document.getElementById("plot").addEventListener("click", function () {
  const testing = document.getElementById('testing').value;
  const rcontactpoint = document.getElementById('rcontactpoint').value;
  const lcontactpoint = document.getElementById('lcontactpoint').value;
  fetch('/plot?testing=' + encodeURIComponent(testing) + '&rcontactpoint=' + encodeURIComponent(rcontactpoint) + '&lcontactpoint=' + encodeURIComponent(lcontactpoint))
      .then(response => response.text())
      .then(data => {
          const parts = data.split(',').map(part => part.trim());

          if (parts.length == 3) {
            const direction = parts[0];
            const x = parseFloat(parts[1]);
            const y = parseFloat(parts[2]);

            if (direction == "R") {
              const dataset = graph1.data.datasets[0].data;
              const existingPoint = dataset.find(point => point.x === x);

              if (existingPoint) {
                existingPoint.y = y;
              } else {
                dataset.push({ x: x, y: y });
              }
              graph1.update();
            }

            else if (direction == "L") {
              const dataset = graph2.data.datasets[0].data;
              const existingPoint = dataset.find(point => point.x === x);

              if (existingPoint) {
                existingPoint.y = y;
              } else {
                dataset.push({ x: x, y: y });
              }
              graph2.update();
            }
          }
          else if (parts.length == 6) {
            const groupedData = []; 
            for (let i = 0; i < parts.length; i += 3) {
              const group = {
                direction: parts[i],
                x: parseFloat(parts[i + 1]),
                y: parseFloat(parts[i + 2])
              };
              groupedData.push(group);
            }

            const g1data = graph1.data.datasets[0].data;
            const g2data = graph2.data.datasets[0].data;
            const existingPoint1 = g1data.find(point => point.x === x);
            const existingPoint2 = g2data.find(point => point.x === x);

            if (existingPoint1) {
              existingPoint1.y = y;
            } else {
              g1data.push({x: groupedData[0].x, y: groupedData[0].y});
            }

            if (existingPoint2) {
              existingPoint2.y = y;
            } else {
              g2data.push({x: groupedData[1].x, y: groupedData[1].y});
            }

            graph1.update();
            graph2.update();
          }
      })
      .catch(error => console.error('Error:', error));
});

document.getElementById("savegraph1").addEventListener("click", function () {
  const image = graph1.toBase64Image();
  const link = document.createElement("a");
  link.href = image;
  link.download = "graph1.png";
  link.click();
});

document.getElementById("savegraph2").addEventListener("click", function () {
  const image = graph2.toBase64Image();
  const link = document.createElement("a");
  link.href = image;
  link.download = "graph2.png";
  link.click();
});

setInterval(function () {
  fetch("/sensor")
    .then((response) => response.text())
    .then((data) => {
      document.getElementById("position").innerText = data;
    });
}, 50);

document.getElementById("cleargraph1").addEventListener("click", function () {
  graph1.data.datasets[0].data = [];
  graph1.update();
});

document.getElementById("cleargraph2").addEventListener("click", function () {
  graph2.data.datasets[0].data = [];
  graph2.update();
});