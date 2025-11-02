import { DynamoDBClient } from '@aws-sdk/client-dynamodb';
import { DynamoDBDocumentClient, PutCommand } from '@aws-sdk/lib-dynamodb';

const client = DynamoDBDocumentClient.from(new DynamoDBClient({}));

export const handler = async (event) => {
  try {
    const params = new URLSearchParams(event.body);
    
    const timestamp = parseInt(params.get('time'));
    const temperature = parseFloat(params.get('t'));
    const humidity = parseFloat(params.get('h'));
    const co2 = parseFloat(params.get('ppm'));
    
    await client.send(new PutCommand({
      TableName: process.env.TABLE_NAME,
      Item: {
        deviceId: 'esp32-001',
        timestamp,
        temperature,
        humidity,
        co2,
        ttl: Math.floor(Date.now() / 1000) + (365 * 86400)
      }
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
