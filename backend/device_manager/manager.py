import logging
from datetime import datetime
from typing import Optional
import numpy as np

from fastapi import FastAPI, UploadFile, HTTPException, Request
from fastapi.responses import PlainTextResponse
from fastapi.exceptions import RequestValidationError
from fastapi.middleware.cors import CORSMiddleware
from starlette.exceptions import HTTPException as StarletteHTTPException

import azure.functions as func

from azure.storage.blob import BlobServiceClient, BlobClient
from azure.core.exceptions import ResourceNotFoundError
from azure.cosmos import CosmosClient, ContainerProxy

from pydantic import BaseModel

class DeviceConfig(BaseModel):
    secconds_2_sleep: int
    upload_mode: str = "test"

#cosmos db
ENDPOINT = "https://esp-cam-nosql.documents.azure.com:443/" #os.environ["COSMOS_ENDPOINT"]
KEY = "A0jFcXsFIparvkIrBGIvWwuaokcWZGG7KcCb0I8lENV4zopgTocteVhUighkSQFDHMR90wQ1K2f0ACDbjyricA==" #os.environ["COSMOS_KEY"]
DATABASE_NAME = "devices-db"
CONTAINER_NAME = "devices"

cosmos_client = CosmosClient(url=ENDPOINT, credential=KEY)
db_client = cosmos_client.get_database_client(DATABASE_NAME)
container_client = db_client.get_container_client(CONTAINER_NAME)

#blob storage
blob_connection_string = "BlobEndpoint=https://fiolkowaresourcesa73d.blob.core.windows.net/;QueueEndpoint=https://fiolkowaresourcesa73d.queue.core.windows.net/;FileEndpoint=https://fiolkowaresourcesa73d.file.core.windows.net/;TableEndpoint=https://fiolkowaresourcesa73d.table.core.windows.net/;SharedAccessSignature=sv=2022-11-02&ss=bfqt&srt=sco&sp=rwdlacupiyx&se=2024-06-27T20:42:20Z&st=2024-01-01T13:42:20Z&spr=https,http&sig=k5d75LATd9tbFvGzscB4AABtKxOUbIF1nujvoU%2F28vg%3D"
blob_service_client = BlobServiceClient.from_connection_string(blob_connection_string)

app = FastAPI()

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"]
)

@app.exception_handler(HTTPException)
async def http_exception_handler(request, exc):
    return func.HttpResponse(str(exc.detail), status_code=exc.status_code)

@app.exception_handler(StarletteHTTPException)
async def http_exception_handler(request, exc):
    return PlainTextResponse(str(exc.detail), status_code=exc.status_code)


@app.exception_handler(RequestValidationError)
async def validation_exception_handler(request, exc):
    return PlainTextResponse(str(exc), status_code=400)



@app.post("/{device_id}/image")
def upload_image(file: UploadFile, device_id: str):   
    try:
        device = container_client.read_item(item=device_id, partition_key=device_id)
        device["last_update"] = str(datetime.now())
        container_client.upsert_item(device)
    except ResourceNotFoundError:
        logging.error("device not exist")
        
        #raise func.HttpResponse("device not found", status_code=404)
        return HTTPException(404, "device not exist")
    
    blob = get_blob_client(device_id)
    try:
        blob.upload_blob(file.file)
    except Exception as e:
#        PlainTextResponse("error while uploading blob to the cloud", status_code=400)
        logging.error("probably this file already exist in the container")
        logging.error(e)
        return HTTPException(400, "blob upload error")

    logging.info("Blob saved to cloud")
    container_check_and_cleanup(blob_service_client, device_id)
    return {"msg": f"file uploaded succesfully with name: {blob.get_blob_properties()['name']}"}

@app.get("/{device_id}/config")
def get_device_config(device_id: str):
    try:
        device = container_client.read_item(item=device_id, partition_key=device_id)
    except:
        logging.error("device not exist")
        return func.HttpResponse("not found", status_code=404)  #HTTPException(404, "device not exist")
    #return DeviceConfig(upload_times_per_day=device["upload_times_per_day"])
    return DeviceConfig(**device)

@app.post("/init_device")
def new_device():
    new_id = get_new_device_id(container_client)
    new_blob_container = blob_service_client.get_container_client(new_id)
    #create blob container
    new_blob_container.create_container()
    return {"device_id": new_id}

@app.get("/test")
def test():
    return {"msg": "elo mordo1!!"}

def get_blob_client(device_id: str) -> BlobClient:
    return blob_service_client.get_blob_client(get_container_name(device_id), get_filename(device_id))

def get_container_name(device_id: str) -> str:
    return device_id

def get_filename(device_id: str) -> str:
    return str(datetime.now()) + ".jpg"

def get_new_device_id(client: ContainerProxy) -> str:
    all_devices = client.read_all_items()
    ids = [int(device["id"][1:]) for device in all_devices]
    new_device_id = "a" + str(max(ids) +1)
    new_device = {"id": new_device_id, "last_update": str(datetime.now()), "upload_times_per_day": "24", 
                "upload_mode": "test"}
    client.create_item(new_device)
    return new_device_id

def container_check_and_cleanup(blob_service_client: BlobServiceClient, device_id: str):
    blob_container_client = blob_service_client.get_container_client(device_id)

    #all_blobs = blob_container_client.list_blob_names()
    all_blobs = [b for b in blob_container_client.list_blobs()]
    modify_dates = [b.last_modified for b in all_blobs]
    blobs_left = len(modify_dates)
    while(blobs_left > 10):
        min_arg = np.argmin(modify_dates)
        print(all_blobs[min_arg].name)
        blob_service_client.get_blob_client(device_id, all_blobs[min_arg].name).delete_blob(delete_snapshots="include")
        blobs_left = blobs_left-1
        del modify_dates[min_arg]
        del all_blobs[min_arg]


