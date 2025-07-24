const mechSelect = document.getElementById("mechanism-select");
const mechFields = {
    belt: document.getElementById("belt-fields"),
    lead_screw: document.getElementById("lead_screw-fields"),
    rack_pinion: document.getElementById("rack_pinion-fields")
};

mechSelect.addEventListener("change", () => {
    const selected = mechSelect.value;
    for (const key in mechFields) {
        mechFields[key].style.display = key === selected ? "block" : "none";
    }
});

document.getElementById("config-form").addEventListener("submit", function (e) {
    e.preventDefault();
    const formData = new FormData(e.target);

    const config = {
        serialMonitorBaud: formData.get("serialBaud") || "115200",
        screenBaud: formData.get("screenBaud") || "9600",
        motorPulsesPerRevolution: formData.get("motorPulses") || "1000",
        motorShaftVelocity: (formData.get("motorShaftVel")) || 1000,
        motorShaftAcceleration: (formData.get("motorShaftAccel")) || 10000,
        defaultUnits: formData.get("defaultUnits"),
        screenType: formData.get("screenType"),
        mechanism: formData.get("mechanism"),
        mechanismParameters: {}
    };


    switch (config.mechanism) {
        case "belt":
            config.mechanismParameters = {
                pulleyDiameter: formData.get("beltPulleyDiameter"),
                unit: formData.get("beltPulleyUnit"),
                gearboxReduction: formData.get("beltGearbox") || "1:1"
            };
            break;
        case "lead_screw":
            config.mechanismParameters = {
                pitch: formData.get("leadPitch"),
                unit: formData.get("leadPitchUnit"),
                gearboxReduction: formData.get("leadGearbox") || "1:1"
            };
            break;
        case "rack_pinion":
            config.mechanismParameters = {
                pinionDiameter: formData.get("pinionDiameter"),
                unit: formData.get("pinionUnit"),
                gearboxReduction: formData.get("rackGearbox") || "1:1"
            };
            break;
    }

    const json = JSON.stringify(config, null, 2);
    const blob = new Blob([json], { type: "application/json" });
    const url = URL.createObjectURL(blob);

    const link = document.getElementById("downloadLink");
    link.href = url;
    link.download = "config.txt";
    link.style.display = "block";
    link.textContent = "⬇️ Download json config file";
});
