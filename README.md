# Flood Monitoring System

<u><h4>FBISE STEM Project 2025</h4></u>
<p>IoT project to alert authorities in case of flood.</p>

<h2>Working</h2>
<p>Constantly monitor river or other water body. Whenever water levels goes beyond critical level or starts overflowing, contact the near authorities (police, ambulances).</p>

<h1>Practical implementation plan (competition-ready, concise)</h1>

<h3>Core modules</h3>
<ol>
<li>Sensing</li>
    <ul>
        <li>Primary: Ultrasonic distance sensor (HC-SR04) or a waterproof pressure transducer (better for real river installations).</li>
        <li>Optional: float switch or ultrasonic + camera for cross-check.</li>
    </ul>
<li>Controller</li>
    <ul>
        <li>Arduino Uno (or ESP32 for built-in Wi-Fi if you want cloud logging).</li>
    </ul>
<li>Communications</li>
    <ul>
        <li>GSM module (SIM800) for SMS and call.</li>
        <li>Use a standard Pakistan SIM with credit.</li>
    </ul>
<li>Decision logic</li>
    <ul>
        <li>Threshold check → require N consecutive readings (user specified 5–20).</li>
        <li>Add hysteresis and time windows (e.g., consecutive over threshold within 10–30 minutes).</li>
    </ul>
<li>Alert & actuation</li>
    <ul>
        <li>Stage 1: SMS to authorities (display SMS log).</li>
        <li>Stage 2: If continues, initiate automated call (GSM voice call) and turn ON LEDs + buzzer.</li>
        <li>Stage 3: Activate a model gate with DC motor/linear actuator (12 V) to open channel to model dam.</li>
    </ul>
<li>Energy</li>
    <ul>
        <li>Small solar panel + 12 V battery or LiFePO4 + charge controller (demonstrate resilience).</li>
    </ul>
<li>Safety & override</li>
    <ul>
        <li>Big red STOP button (manual override).</li>
        <li>Watchdog & power-fail behavior: gates default to safe position.</li>
    </ul>
</ol>

<h3>Suggested sensors & parts (student-friendly)</h3>
<ul>
    <li>Ultrasonic module (or water-pressure sensor) — 1–2</li>
    <li>Arduino Uno or ESP32</li>
    <li>GSM module (SIM800L)</li>
    <li>Relay module or motor driver (L298N or TB6612)</li>
    <li>12 V DC linear actuator or geared DC motor + gearbox</li>
    <li>LEDs, buzzer, pushbuttons</li>
    <li>Small solar panel (10–20 W) + battery + charge controller</li>
    <li>Enclosure & model river channel (acrylic or plywood)</li>
</ul>

<h3>Software logic (high level)</h3>
<ul>
    <li>Read level every X seconds.</li>
    <li>If reading > threshold → increment counter, else reset (or decrement slowly).</li>
    <li>If counter ≥ N1 (e.g., 5) → send SMS, turn on LEDs.</li>
    <li>If counter ≥ N2 (e.g., 20) or sustained for T minutes → initiate automatic call + sound buzzer + open gate.</li>
    <li>Log events to SD or serial (for judges to inspect).</li>
</ul>

<h3>Testing & demo plan (what to show)</h3>
<ol>
    <li>Tabletop river model with controllable inflow (small pump). Show baseline safe level.</li>
    <li>Gradually raise water level; display live sensor reading on screen (LCD or laptop).</li>
    <li>When threshold reached: show SMS received on phone (mirror on display), LED lights turn on.</li>
    <li>If continued: show automated call being placed (play recorded audio), buzzer + LEDs intensify.</li>
    <li>Gate opens; water diverts to model dam; measure diverted volume (graduated cylinder) and show data.</li>
    <li>Show logs/graphs: time vs level, alerts issued, decision thresholds, false-positive testing.</li>
    <li>Explain scaling: how model maps to real river (scaling factors, flow rates).</li>
</ol>

<h3>How to maximize impressions</h3>
<ul>
    <li><strong>Clarity of physics:</strong> explicitly explain fluid flow, threshold logic, sensor error and debouncing, actuator dynamics.</li>
    <li><strong>Data & reproducibility:</strong> include repeatable trials, failure modes tested, false-positive stats.</li>
    <li><strong>Societal impact:</strong> show how diversion would help small dams in Pakistan (volume estimates) and include stakeholder map (local authorities, NGOs).</li>
    <li><strong>Safety & ethics:</strong> explain manual override, why full-scale deployment requires inter-agency permission.</li>
    <li><strong>Polish:</strong> neat enclosure, labeled wiring, working UI (LCD/graph), printed poster with diagrams and block diagram.</li>
</ul>

<h3>Possible weaknesses & how to mitigate them</h3>
<ul>
    <li><strong>False alarms:</strong> mitigated by multi-sensor confirmation and requiring consecutive readings.</li>
    <li><strong>Connectivity issues:</strong> implement retry & fallback (store alerts in SD, visual alarm if GSM fails).</li>
    <li><strong>Gate safety/legalities:</strong> always present the model as a prototype and include a plan for stakeholder coordination for real deployment.</li>
    <li><strong>Power failure:</strong> solar + battery backup demonstrated.</li>
</ul>
