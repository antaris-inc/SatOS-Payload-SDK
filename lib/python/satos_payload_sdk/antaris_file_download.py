from azure.storage.fileshare import ShareFileClient
import logging
import requests
import os

logger = logging.getLogger("azure.core.pipeline.policies.http_logging_policy")
logger.setLevel(logging.WARNING)

gHttpOk = 200
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
        connection_string = self.config_data[g_FTM][g_File_String]
        
        if "FileEndpoint=" in connection_string:
            ret = azure_file_upload(file_path_local, connection_string, self.config_data[g_FTM][g_Share_Name], self.file_path_remote)
            return ret
        elif "gcs-bucket-upload" in connection_string:
            return gcp_file_upload(
                endpoint=connection_string,
                file_path=file_path_local,
                gcs_bucket=self.config_data[g_FTM][g_Share_Name],
                truetwin_dir=self.config_data[g_FTM][g_Truetwin_Dir],
                filename=self.download_file_params.file_path
            )
        else:
            raise ValueError("Unsupported connection string format")

    def file_download(self):
        return self.start_upload()

def azure_file_upload(file_name, conn_str, share_name, file_path):
    file_client = ShareFileClient.from_connection_string(conn_str, share_name, file_path)
    try:
        with open(file_name, "rb") as source_file:
            file_client.upload_file(source_file)
            return True
    except Exception as e:
        logger.error("Upload to truetwin failed")
        logger.error(f"Error message: {str(e)}")
        return False

def gcp_file_upload(endpoint, file_path, gcs_bucket, truetwin_dir, filename):
    try:
        with open(file_path, 'rb') as f:
            files = {
                'file': (filename, f),
            }
            data = {
                'gcs_bucket': gcs_bucket,
                'truetwin_dir': truetwin_dir,
                'filename': filename,
            }
            response = requests.post(endpoint, files=files, data=data)

        if response.status_code == gHttpOk:
            return True
        else:
            logger.error("Upload to GCS failed")
            logger.error(f"Status Code: {response.status_code}")
            logger.error(f"Response Text: {response.text}")
            return False

    except Exception as e:
        logger.error("Exception during GCP file upload")
        logger.error(f"Error message: {str(e)}")
        return False