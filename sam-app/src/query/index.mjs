import { DynamoDBClient } from '@aws-sdk/client-dynamodb';
import { DynamoDBDocumentClient, QueryCommand } from '@aws-sdk/lib-dynamodb';

const client = DynamoDBDocumentClient.from(new DynamoDBClient({}));

export const handler = async (event) => {
  try {
    const range = event.queryStringParameters?.range || '24h';
    const deviceId = event.queryStringParameters?.deviceId || 'esp32-001';
    
    const rangeMap = {
      '24h': 24 * 60 * 60 * 1000,
      '7d': 7 * 24 * 60 * 60 * 1000,
      '30d': 30 * 24 * 60 * 60 * 1000
    };
    
    const now = Date.now();
    const startTime = now - (rangeMap[range] || rangeMap['24h']);
    
    const result = await client.send(new QueryCommand({
      TableName: process.env.TABLE_NAME,
      KeyConditionExpression: 'deviceId = :deviceId AND #ts >= :startTime',
      ExpressionAttributeNames: {
        '#ts': 'timestamp'
      },
      ExpressionAttributeValues: {
        ':deviceId': deviceId,
        ':startTime': startTime
      }
    }));
    
    const data = result.Items.map(item => ({
      timestamp: item.timestamp,
      temperature: item.temperature,
      humidity: item.humidity,
      co2: item.co2
    }));
    
    return {
      statusCode: 200,
      headers: {
        'Content-Type': 'application/json',
        'Access-Control-Allow-Origin': '*'
      },
      body: JSON.stringify({ data })
    };
  } catch (error) {
    console.error('Error:', error);
    return {
      statusCode: 500,
      headers: {
        'Content-Type': 'application/json',
        'Access-Control-Allow-Origin': '*'
      },
      body: JSON.stringify({ error: 'Internal server error' })
    };
  }
};
