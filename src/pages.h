const char index_page[] PROGMEM = R"=====(
<html>
  <head>
    <style>
      table.customTable {
        background-color: #FFFFFF;
        border-collapse: collapse;
        border-width: 2px;
        border-color: #7EA8F8;
        border-style: solid;
        color: #000000;
      }
    
      table.customTable td, table.customTable th {
        border-width: 2px;
        border-color: #7EA8F8;
        border-style: solid;
        padding: 5px;
       }
    
      table.customTable thead {
        background-color: #7EA8F8;
      }
    </style>
  </head>
  <body>
    <h1>Modbus Bridge for outside temperature sensor</h1>
    <h2>Version: 0.1 </h2>
    <h2>States</h2>
    <table class="customTable">
      <thead>
        <tr>
          <th>Register address</th>
          <th>Register Name</th>
          <th>Raw HEX</th>
          <th>Value</th>
          <th>Unit</th>
        </tr>
      </thead>
      <tbody>
        %MODBUS_REGISTER_DATA%
      </tbody>
    </table>


    <h2>OTA Update</h2>
    <form method="POST" action="/update" enctype="multipart/form-data">
      <input type="file" name="update">
      <input type="submit" value="Update">
    </form>
  </body>
</html>
)=====";

const char page_404[] PROGMEM = R"=====(
404 Not Found
)=====";
