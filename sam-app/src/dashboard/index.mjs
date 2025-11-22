import { S3Client, GetObjectCommand } from '@aws-sdk/client-s3';

const s3 = new S3Client({});

export const handler = async (event) => {
  try {
    // Determine which file to serve
    const path = event.rawPath || event.path || '/';
    const fileName = path === '/favicon.png' ? 'favicon.png' : 'index.html';
    const contentType = fileName === 'favicon.png' ? 'image/png' : 'text/html';
    
    const result = await s3.send(new GetObjectCommand({
      Bucket: process.env.BUCKET_NAME,
      Key: fileName
    }));
    
    if (fileName === 'favicon.png') {
      // Return binary data for favicon
      const chunks = [];
      for await (const chunk of result.Body) {
        chunks.push(chunk);
      }
      const buffer = Buffer.concat(chunks);
      
      return {
        statusCode: 200,
        headers: {
          'Content-Type': contentType
        },
        body: buffer.toString('base64'),
        isBase64Encoded: true
      };
    } else {
      // Return HTML as text
      const html = await result.Body.transformToString();
      
      return {
        statusCode: 200,
        headers: {
          'Content-Type': contentType
        },
        body: html
      };
    }
  } catch (error) {
    console.error('Error:', error);
    return {
      statusCode: 500,
      body: 'Error loading resource'
    };
  }
};
