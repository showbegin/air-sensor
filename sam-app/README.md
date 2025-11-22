# CO2 Sensor AWS Backend

## Project Structure

```
sam-app/
├── template.yaml           # SAM template with all AWS resources
├── samconfig.toml          # Deployment configuration
├── deploy.sh               # Deployment script
├── README.md               # Documentation
├── src/
│   ├── ingest/            # Lambda for ESP32 data ingestion
│   │   ├── index.mjs
│   │   └── package.json
│   ├── query/             # Lambda for dashboard data queries
│   │   ├── index.mjs
│   │   └── package.json
│   └── dashboard/         # Lambda for serving dashboard HTML
│       ├── index.mjs
│       └── package.json
└── dashboard/             # Static dashboard HTML (uploaded to S3)
    └── index.html
```

## Resources Created

- **DynamoDB Table**: `co2-sensor-data` (on-demand billing, 1-year TTL)
- **Lambda Functions**: Ingest (ESP32), Query (Dashboard API), Dashboard (HTML serving)
- **API Gateway**: HTTP API with custom domain
- **S3 Bucket**: Dashboard HTML storage
- **ACM Certificate**: SSL for co2.arsen.ganibek.com
- **Route53 Record**: DNS for custom domain
- **S3 Bucket**: Dashboard hosting
- **CloudFront**: CDN with custom domain
- **ACM Certificate**: SSL for co2.arsen.ganibek.com
- **Route53 Record**: DNS alias to CloudFront

## Deployment

### Prerequisites
- AWS SAM CLI installed
- Node.js 20+ installed
- AWS profile 'personal' configured

### Deploy

```bash
cd sam-app
./deploy.sh
```

Or manually:
```bash
cd src/ingest && npm install && cd ../..
cd src/query && npm install && cd ../..
sam build
sam deploy
```

### Outputs

After deployment, note these outputs:
- `IngestApiUrl`: https://co2.arsen.ganibek.com/ingest (Update ESP32 sketch)
- `QueryApiUrl`: https://co2.arsen.ganibek.com/data (Used by dashboard)
- `DashboardUrl`: https://co2.arsen.ganibek.com/ (Public dashboard)
- `DashboardBucketName`: Upload dashboard files here

## Next Steps

### Phase 2: Update ESP32
Replace endpoint in sketch:
```cpp
https.begin("<IngestApiUrl>");
```

### Phase 3: Deploy Dashboard
```bash
aws s3 cp dashboard/index.html s3://<DashboardBucketName>/ --profile personal
```

Dashboard is automatically served at https://co2.arsen.ganibek.com/

## Cost Estimate

~$0.80/month:
- API Gateway: $0.26
- Lambda: Free tier
- DynamoDB: Free tier
- S3: $0.02
- CloudFront: Free tier
- Route53: $0.50
