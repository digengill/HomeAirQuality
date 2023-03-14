var sqlite3 = require('sqlite3').verbose()

const DBSOURCE = "db.sqlite"

let db = new sqlite3.Database(DBSOURCE, (err) => {
    if (err) {
      // Cannot open database
      console.error(err.message)
      throw err
    }else{
        console.log('Connected to the SQLite database.')
        db.run(`CREATE TABLE ESPReadings (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            hostname text,
            dt text, 
            temp REAL,
            humidity REAL,  
            voc INTEGER,
	    gasresistance INTEGER;  
            )`,
        (err) => {
            if (err) {
                // Table already created
		console.log(err)
            }else{
                // Table just created, creating some rows
                //var insert = 'INSERT INTO ESPReadings (hostname, dt, temp, humidity, voc) VALUES (?,?,?,?,?)'
                //db.run(insert, ["test","2023-03-07 22:05:00.000", 0.0, 0.0, 0])
            }
        });  
    }
});


module.exports = db
