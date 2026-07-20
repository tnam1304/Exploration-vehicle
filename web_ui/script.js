/**
 * Processes and forwards control packets to the execution log.
 * @param {string} cmd - The operational flag ('F', 'B', 'L', 'R', 'S', 'P')
 */
function sendCmd(cmd) {
  // Logs the active telemetry packet command directly to the developer console
  console.log("Command sent: " + cmd);
  
  // Triggers hardware haptic motors for a 35ms pulse (Android web only)
  if (navigator.vibrate) navigator.vibrate(35); 
}

/**
 * Initializes the native browser Web Speech engine and handles Vietnamese vocal recognition parsing.
 */
function startSpeechRecognition() {
  // Cross-browser compatibility check
  const SpeechRecognition = window.SpeechRecognition || window.webkitSpeechRecognition;
  
  if (!SpeechRecognition) {
    alert("Speech recognition not supported. Please use Google Chrome.");
    return;
  }

  // Create an operational instance of the vocal engine
  const recognition = new SpeechRecognition();
  recognition.lang = "vi-VN";         
  recognition.interimResults = false; 

  const voiceText = document.getElementById("voice-text");
  voiceText.innerText = "🎙️ Listening...";
  voiceText.style.color = "#00ffcc";  

  /**
   * Success callback event executed when voice processing finishes.
   */
  recognition.onresult = function(event) {
    // Extract raw text and map all characters to lowercase
    const result = event.results[0][0].transcript.toLowerCase();
    
    // Display the interpreted voice output onto the dashboard interface
    voiceText.innerText = 'You said: "' + result + '"';
    voiceText.style.color = "#fff";   

    // Core intent parsing logic: Checks for movement and parking keywords
    if (result.includes("tiến") || result.includes("thẳng") || result.includes("chạy")) {
      sendCmd("F"); 
    } else if (result.includes("lùi") || result.includes("sau")) {
      sendCmd("B"); 
    } else if (result.includes("trái")) {
      sendCmd("L"); 
    } else if (result.includes("phải")) {
      sendCmd("R"); 
    } else if (result.includes("dừng") || result.includes("lại") || result.includes("stop")) {
      sendCmd("S"); 
    } else if (result.includes("đỗ") || result.includes("đỗ xe") || result.includes("parking")) {
      sendCmd("P"); // Map to Auto Parking command flag
    } else {
      voiceText.innerText += " (Unknown command, try again)";
      voiceText.style.color = "#ff5252"; 
    }
  };

  /**
   * Error catch block executed if issues arise with permissions or microphone.
   */
  recognition.onerror = function(event) {
    voiceText.innerText = "Error: " + event.error;
    voiceText.style.color = "#ff5252"; 
  };

  // Energize microphone hardware and begin streaming audio data
  recognition.start();
}

/* Prevent native context dropdown menu triggers during prolonged mobile touch holds */
document.addEventListener('contextmenu', event => event.preventDefault());

/* Intercept mobile browser vertical swipe gestures to lock desktop interface position alignment */
document.addEventListener('touchmove', event => event.preventDefault(), {passive: false});