#!/bin/bash
set -e

echo "Installing Lambda dependencies..."
cd src/ingest && npm install && cd ../..
cd src/query && npm install && cd ../..

echo "Building SAM application..."
sam build

echo "Deploying to AWS..."
sam deploy

echo "Deployment complete!"
echo "Check outputs for API endpoints and dashboard URL"
