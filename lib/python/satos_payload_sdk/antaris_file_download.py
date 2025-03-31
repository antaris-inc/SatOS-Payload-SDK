from azure.storage.fileshare import ShareFileClient
import logging
import datetime
import requests
# from google.cloud import storage
from google.oauth2 import service_account

logger = logging.getLogger("azure.core.pipeline.policies.http_logging_policy")
logger.setLevel(logging.WARNING)

g_FTM = "FTM"
g_File_String = "File_Conn_Str"
g_Truetwin_Dir = "Truetwin_Dir"
g_Share_Name = "Share_Name"
g_Outbound_Path_Prefix = "/opt/antaris/outbound/"

class File_Stage():

    def __init__(self, download_file_params,config_data ): 
        self.download_file_params = download_file_params
        self.config_data = config_data
        self.file_name = self.download_file_params.file_path
        self.file_path_remote = self.config_data[g_FTM][g_Truetwin_Dir] + "/" + self.file_name

    def start_upload(self):
        file_path_local = g_Outbound_Path_Prefix + self.download_file_params.file_path
        # ret = azure_file_upload(file_path_local, self.config_data[g_FTM][g_File_String], self.config_data[g_FTM][g_Share_Name], self.file_path_remote)
        ret = gcp_file_upload(self.config_data[g_FTM][g_File_String], file_path_local)
        return ret
        
    def file_download(self):
        if  self.start_upload():
            return True
        else:
            return False
        
def azure_file_upload(file_name, conn_str, share_name, file_path):
    file_client = ShareFileClient.from_connection_string(conn_str, share_name, file_path)
    try:
        with open(file_name, "rb") as source_file:
            file_client.upload_file(source_file)
            return True
    except Exception as e:
        logger.error("Upload  to truetwin failed")
        logger.error(f"Error message: {str(e)}")
        return False
    
def gcp_file_upload(signed_url, local_file_path):
    """Uploads a file using the signed URL."""
    with open(local_file_path, "rb") as file:
        response = requests.put(signed_url, data=file, headers={"Content-Type": "application/octet-stream"})

    if response.status_code == 200:
        print("✅ File uploaded successfully!")
    else:
        print(f"❌ Upload failed. Status: {response.status_code}, Response: {response.text}")