#ifndef CONFIG_WEBPAGE_H_
#define CONFIG_WEBPAGE_H_

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <title>PC Switch Setup</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      body {
        font-family: sans-serif;
        max-width: 400px;
        margin: 50px auto;
        text-align: center;
      }
      input, button {
        padding: 10px;
        margin: 10px;
        width: 90%;
      }
      label {
        display: block;
        margin-top: 15px;
        text-align: left;
      }
      .radio-group {
        display: flex;
        justify-content: space-around;
        margin-top: 10px;
      }
    </style>
  </head>
  <body>
    <h1>PC Switch Setup</h1>
    <form method="POST" action="/save">
      <input type="text" name="ssid" placeholder="WiFi SSID" required><br>
      <input type="password" name="password" placeholder="WiFi Password" required><br>

      <label>Operation Mode:</label>
      <div class="radio-group">
        <label><input type="radio" name="opmode" value="0" checked> HTTP</label>
        <label><input type="radio" name="opmode" value="1"> HomeKit</label>
      </div>

      <button type="submit">Save and Reboot</button>
    </form>
  </body>
</html>
)rawliteral";

const char saved_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <title>PC Switch Setup</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      body {
        font-family: sans-serif;
        max-width: 400px;
        margin: 50px auto;
        text-align: center;
      }
    </style>
  </head>
  <body>
    <h1>PC Switch Setup</h1>
    <h2>Configuration Saved</h2>
    <p>Restarting device...</p>
  </body>
  <script>
    setTimeout(function(){ location.href = '/'; }, 1000);
  </script>
</html>
)rawliteral";

#endif
