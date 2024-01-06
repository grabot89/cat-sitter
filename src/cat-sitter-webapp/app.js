const express = require('express');
const mongoose = require('mongoose');
const mqtt = require('mqtt');

const app = express();
const port = 3000;

// Serve static files
app.use(express.static('public'));

// Connect to MongoDB
mongoose.connect('mongodb://localhost/sensorData', { useNewUrlParser: true, useUnifiedTopology: true });

// Define MongoDB schema and model
const sensorDataSchema = new mongoose.Schema({
  _id: mongoose.Schema.Types.ObjectId,
  deviceID: String,
  temp: Number,
  humidity: Number,
  timestamp: Date,
  forbidden: Boolean,
});

const SensorData = mongoose.model('SensorData', sensorDataSchema);
SensorData.createCollection()
  .then(() => {
    console.log('MongoDB collection created');
  })
  .catch((error) => {
    console.error('Error creating MongoDB collection:', error);
  });

// SensorData.deleteMany({})
//   .then((result) => {
//     console.log('Collection cleared successfully:', result.deletedCount, 'documents deleted');
//   })
//   .catch((err) => {
//     console.error('Error clearing collection:', err);
//   })
//   .finally(() => {
//     // Close the MongoDB connection
//     mongoose.connection.close();
//   });

// MQTT setup
const client  = mqtt.connect('mqtt://broker.emqx.io');
const mqttTopic = '/cat-sitter/measurements';

client.on('connect', () => {
  console.log('Connected to MQTT broker');
  client.subscribe(mqttTopic);
});

client.on('message', (topic, message) => {
  const data = JSON.parse(message.toString());
  const timeStamp = new Date(data.timestamp * 1000);
  console.log("Data: ", data);
  console.log("Date is ", timeStamp);
  
  // Save data to MongoDB
  //const sensorData = new SensorData(data);
  const sensorData = new SensorData({
  _id: new mongoose.Types.ObjectId(), // Generate a new ObjectId manually or the timestamp becomes now, won't use from arduino message
  deviceID: data.deviceID,
  temp: data.temp,
  humidity: data.humidity,
  timestamp: timeStamp,
  forbidden: data.forbidden,
});
  
  sensorData.save()
    .then(() => {
      console.log('Data saved to MongoDB');
    })
    .catch((error) => {
      console.error('Error saving data to MongoDB:', error);
    });
});

// Express routes
app.get('/', (req, res) => {
  res.send('Hello, this is your Express app!');
});

app.get('/chart', async (req, res) => {
  try {
    const data = await SensorData.find().exec();
    res.json(data);
  } catch (error) {
    res.status(500).json({ error: 'Internal Server Error' });
  }
});

// Start the Express app
app.listen(port, () => {
  console.log(`Server is running on port ${port}`);
});
