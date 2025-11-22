import { DynamoDBClient } from '@aws-sdk/client-dynamodb';
import { DynamoDBDocumentClient, PutCommand } from '@aws-sdk/lib-dynamodb';

const client = DynamoDBDocumentClient.from(new DynamoDBClient({}));

export const handler = async (event) => {
  try {
    console.log('Raw event body:', event.body);
    console.log('isBase64Encoded:', event.isBase64Encoded);
    
    // Decode base64 if needed
    let body = event.body;
    if (event.isBase64Encoded) {
      body = Buffer.from(event.body, 'base64').toString('utf-8');
      console.log('Decoded body:', body);
    }
    
    const params = new URLSearchParams(body);
    
    const timestamp = parseInt(params.get('time'));
    const temperature = parseFloat(params.get('t'));
    const humidity = parseFloat(params.get('h'));
    const co2 = parseFloat(params.get('ppm'));
    
    console.log('Parsed values:', { timestamp, temperature, humidity, co2 });
    
    // Validate required values (humidity is optional)
    if (isNaN(timestamp) || isNaN(temperature) || isNaN(co2)) {
      console.error('Invalid data received:', { timestamp, temperature, humidity, co2 });
      return {
        statusCode: 400,
        body: 'Invalid sensor data'
      };
    }
    
    const item = {
      deviceId: 'esp32-001',
      timestamp,
      temperature,
      co2,
      ttl: Math.floor(Date.now() / 1000) + (365 * 86400)
    };
    
    // Only add humidity if it's a valid number
    if (!isNaN(humidity)) {
      item.humidity = humidity;
    }
    
    await client.send(new PutCommand({
      TableName: process.env.TABLE_NAME,
      Item: item
    }));
    
    return {
      statusCode: 200,
      body: 'OK'
    };
  } catch (error) {
    console.error('Error:', error);
    return {
      statusCode: 500,
      body: 'Error'
    };
  }
};
