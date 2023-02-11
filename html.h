const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>SoyoSource Web-Limiter</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    table, td, th { border: 1px solid white; }
    table.font1{ font-size: 0.8rem; }
    td { text-align: left; }
    th { text-align: center; }
    p {  font-size: 1.2rem;}
    body {  margin: 0;}
    .topnav { overflow: hidden; background-color: #263c5e; color: white; font-size: 1.7rem; }
    .content { padding: 20px; }
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
    .card-grid { max-width: 700px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); }
    .reading { font-size: 2.0rem; }
    .headline { font-size: 2.0rem; font-weight: bold; }
    .card.info { color: #034078; }
    .card.info-dark { color: #151515; }
    .card-title { font-size: 1.2rem; font-weight: bold; color: #034078; }
    .card-title-dark { font-size: 1.2rem; font-weight: bold; color: #151515; }
    .btn:active { transform: scale(0.80); box-shadow: 3px 2px 22px 1px rgba(0, 0, 0, 0.24); }
    .btn {
            text-decoration: none;
            border: none;
            width: 90px;
            padding: 12px 20px;
            margin: 15px 5px 15px 5px;
            text-align: center;
            font-size: 16px;
            background-color:#034078;
            color: #fff;
            border-radius: 5px;
            box-shadow: 7px 6px 28px 1px rgba(0, 0, 0, 0.24);
            cursor: pointer;
            outline: none;
            transition: 0.2s all;
          }
  </style>

</head>
<body>

  <div class="topnav">
    <h3>SoyoSource Web-Limiter</h3>
  </div>

  <div class="content">
    <div class="card-grid">
      <div class="card">
          <p class="card-title-dark">ESP Infos</p>
          <table style="width:100%%" class="font1 center">
            <tbody>
              <tr>
                <td width="65">ClientID:</td>
                <td width="100" id="client_id">%CLIENTID%</td>
                <th></th>
                <td width="50">Uptime:</td>
                <td width="140" id="up_time">%UPTIME%</td>
              </tr>
              <tr>
                <td>Wifi RSSI:</td>
                <td><span><span id="wifi_rssi">%WIFIRSSI%</span> dB</span></td>
                <th></th>
                <td></td>
                <td></td>
              </tr>
            </tbody>
          </table>
      </div>
    </div>
  </div>

  <div class="content">
    <div class="card-grid">
      <div class="card">
        <p class="card-title"><span class="headline">AC Output: </span><span class="reading"><span id="static_power">%STATICPOWER%</span> W</span></p>
        <hr style="color: #fafafa;">        
        <table style="width:100%%" class="center">
          <tbody>
            <tr>
              <th><button type="button" onclick="minus1();" class="btn">- 1</button></th>
              <th class="card-title">Set Output Power</th>
              <th><button type="button" onclick="plus1();" class="btn">+ 1</button></th>
            </tr>
            <tr>
              <th><button type="button" onclick="minus10();" class="btn">- 10</button></th>
              <th><button type="button" onclick="set_0();" class="btn">0</button></th>
              <th><button type="button" onclick="plus10();" class="btn">+ 10</button></th>
            </tr>
          </tbody>
        </table>
      </div>
    </div>
  </div>

      
  <div class="content">
    <div class="card-grid">
      <div class="card">
        <table style="width:100%%">
          <tbody>          
            <tr>
              <th style="text-align: right"><a href="update"><button type="button"  class="btn" style="width: auto; margin: 5px 5px 5px 5px; background-color:#767676;"> FW Update</button></a></th>
            </tr>
          </tbody>
        </table>
      </div>
    </div>
  </div>

  <div class="content">
    <!--<p>Message from SoyoSource</p> -->
    <table style="width:100%%">
      <tbody>          
        <tr>
          <td id="soyo_text">%SOYOTEXT%</td>
        </tr>
      </tbody>
    </table>
  </div>

<script>
  if (!!window.EventSource) {
    var source = new EventSource('/events');

    source.addEventListener('open', function(e) {
      console.log("Events Connected");
    }, false);

    source.addEventListener('error', function(e) {
      if (e.target.readyState != EventSource.OPEN) {
        console.log("Events Disconnected");
      }
    }, false);
    
    source.addEventListener('message', function(e) {
      console.log("message", e.data);
    }, false);
    
    source.addEventListener('staticpower', function(e) {
      console.log("staticpower", e.data);
      document.getElementById('static_power').innerHTML = e.data;
    }, false);

    source.addEventListener('wifirssi', function(e) {
      console.log("wifirssi", e.data);
      document.getElementById('wifi_rssi').innerHTML = e.data;
    }, false);

    source.addEventListener('clientid', function(e) {
      console.log("clientid", e.data);
      document.getElementById('client_id').innerHTML = e.data;
    }, false);

    source.addEventListener('uptime', function(e) {
      console.log("uptime", e.data);
      document.getElementById('up_time').innerHTML = e.data;
    }, false);

     source.addEventListener('soyotext', function(e) {
      console.log("soyotext", e.data);
      document.getElementById('soyo_text').innerHTML = e.data;
    }, false);
  
  }

  function set_0() {
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "/set_0", true);
    xhttp.send();
  };

  function minus1 () {
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "/minus1", true);
    xhttp.send();
  };

  function minus10 () {
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "/minus10", true);
    xhttp.send();
  };

  function plus1 () {
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "/plus1", true);
    xhttp.send();
  };

  function plus10 () {
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "/plus10", true);
    xhttp.send();
  };

</script>

</body>
</html>)rawliteral";