const express = require('express')
const app = express()
const port = 3001

var db = require("./database.js")
var bodyParser = require('body-parser');
app.use(bodyParser.json()); // support json encoded bodies
app.use(bodyParser.urlencoded({ extended: true })); // support encoded bodies


app.get("/readings", (req, res, next) => {
    res.header("Access-Control-Allow-Origin", "*");
    var sql = "select * from ESPReadings"
    var params = []
    db.all(sql, params, (err, rows) => {
        if (err) {
          res.status(400).json({"error":err.message});
          return;
        }
        res.json({
            "message":"success",
            "data":rows
        })
      });
});

app.post('/sgp40', (req, res) => {
  console.log(req.body)
  var sql ='INSERT INTO ESPReadings (hostname, dt, temp, humidity, voc, gasresistance) VALUES (?,?,?,?,?,?)'
  var params =["esp01", new Date().toISOString(), parseFloat(req.body.temp).toFixed(2), parseFloat(req.body.humidity).toFixed(2), req.body.voc, req.body.gasr]
  db.run(sql, params, function (err, result) {
      if (err){
          //res.status(400).json({"error": err.message})
          console.log(err.message)
          return;
      }
    });
})

app.listen(port, '10.0.0.194');
