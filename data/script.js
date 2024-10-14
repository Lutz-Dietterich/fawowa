document.addEventListener("DOMContentLoaded", function () {
  // Update Lüftergeschwindigkeit in Echtzeit anzeigen
  const fanSpeedInput = document.getElementById("fanSpeed");
  const fanSpeedValue = document.getElementById("fanSpeedValue");

  fanSpeedInput.addEventListener("input", function () {
    fanSpeedValue.textContent = fanSpeedInput.value;
  });

  // Funktion zum Abrufen der Sensordaten vom ESP32
  async function fetchSensorData() {
    try {
      const response = await fetch("/data"); // Der Endpunkt für die Sensordaten
      const data = await response.json();

      // Aktualisiere die Innenraumdaten (ohne CO₂ und Außenwerte)
      document.getElementById("tempInside").textContent = data.internal_temperature;
      document.getElementById("humidityInside").textContent = data.internal_humidity;

      // Überprüfe, ob Außensensordaten vorhanden sind
      if (data.external_temperature !== undefined && data.external_temperature !== null) {
        document.getElementById("tempOutside").textContent = data.external_temperature;
      } else {
        document.getElementById("tempOutside").textContent = "Keine Daten";
      }

      if (data.external_humidity !== undefined && data.external_humidity !== null) {
        document.getElementById("humidityOutside").textContent = data.external_humidity;
      } else {
        document.getElementById("humidityOutside").textContent = "Keine Daten";
      }
    } catch (error) {
      console.error("Fehler beim Abrufen der Sensordaten", error);
    }
  }

  // Aktualisiere die Sensordaten alle 5 Sekunden
  setInterval(fetchSensorData, 5000);
  fetchSensorData(); // Erste Daten sofort laden
});
