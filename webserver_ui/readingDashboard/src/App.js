import React, {useState, useEffect} from "react";
import "chartjs-adapter-moment";
import { Line } from "react-chartjs-2";
import axios from "axios";
import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  TimeScale,
  Title,
  Tooltip,
  Legend
} from "chart.js";
import { Chart } from "react-chartjs-2";

ChartJS.register(
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
  TimeScale
);

const options = {
        scales: {
            x: {
                type: 'time',
                time: {
                    unit: 'minutes'
                }
            }
        }
    }



export default function App() {
  const [dt, setDt] = useState([]);
  const [temp, setTemp] = useState([]);
  const [hum, setHum] = useState([]);
  const [voc, setVoc] = useState([]);

 let zip = (a1, a2) => a1.map((x, i) => [x, a2[i]]);

 useEffect( () => {
  axios.get(`http://10.0.0.194:3001/readings`)
  .then(res => {
    console.log(res.data.data);
    let tdt = []
    let ttemp = []
    let thum = []
    let tvoc = []
    res.data.data.forEach(element => {
      tdt.push(element['dt'])
      ttemp.push(element['temp'])
      thum.push(element['humidity'])
      tvoc.push(element['voc'])
    });
    setDt(tdt);
    setTemp(ttemp);
    setHum(thum);
    setVoc(tvoc);
  
  }) 
}, []);


const data = {
  datasets: [
    {
      label: "Temperature (c)",
      data: zip(dt,temp),
      borderColor: 'rgb(255, 99, 132)',
      backgroundColor: 'rgba(255, 99, 132, 0.5)'
    },
    {
	    label: "Humidity (%)",
	    data: zip(dt,hum),
      borderColor: 'rgb(53, 162, 235)',
      backgroundColor: 'rgba(53, 162, 235, 0.5)'
    },
	  {
		label: "VOC",
		data: zip(dt,voc),
    borderColor: 'rgb(23, 162, 135)',
    backgroundColor: 'rgba(12, 12, 232, 0.5)'
	  }
  ]
};

const test = () => {
    console.log(temp);
    console.log(hum);
    console.log(voc);}

  return (
  <div>
    <Line options={options} data={data} redraw={true} />
    <button onClick={()=>{test()}}>click</button>
    </div>
  );
}

