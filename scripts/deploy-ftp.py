#!/usr/bin/env python3

import ftplib
import os
from typing import Any, Dict, Optional, Tuple, Union
from typing_extensions import Self
from dotenv import load_dotenv

# https://ftputil.sschwarzer.net
import ftputil

NoneType = type(None)

# Load environment variables from .env file.
load_dotenv()

# Wrapper to assert the environment variable exists.
def getenv(key: str, default: Optional[str] = None, *args: Tuple[Any], **kwargs: Dict[str, Any]) -> str:
    '''Wrapper for os.getenv.'''
    
    value: Union[None, str] = os.getenv(key)
    # Use the value if it exists.
    if value is not None:
        return value
    # If we're provided a default, fallback to that.
    elif default is not None:
        return default
    # Assert key is missing.
    else:
        raise KeyError(f'Missing environment variable {key}! Only run this script via make deploy-ftp.')


# Get environment variables from build system.
PROGRAM_ID: str = getenv('APP_TITLEID')
FTP_IP: str = getenv('FTP_ADDRESS')
FTP_PORT: int = int(getenv('FTP_PORT'))
FTP_USERNAME: str = getenv('FTP_USERNAME')
FTP_LOCAL_DIR: str = getenv('FTP_LOCAL_DIR')
FTP_REMOTE_DIR: str = getenv('FTP_REMOTE_DIR')
FTP_REMOTE_LOG: str = getenv('FTP_REMOTE_LOG')

# Password may be empty, so special case this.
FTP_PASSWORD: str = getenv('FTP_PASSWORD', '')

class SessionFactory(ftplib.FTP):
    '''Session factory for FTPHost.'''
    
    def __init__(self: Self, ftp_ip: str, ftp_port: int, ftp_username: str, ftp_password: str, *args: Tuple[Any], **kwargs: Dict[str, Any]) -> NoneType:
        super().__init__()
        
        # Connect to FTP server.
        self.connect(ftp_ip, ftp_port)
        # Login with credentials.
        self.login(ftp_username, ftp_password)


def main(*args: Tuple[Any], **kwargs: Dict[str, Any]) -> NoneType:
    # Connect/login to console FTP server.
    with ftputil.FTPHost(FTP_IP, FTP_PORT, FTP_USERNAME, FTP_PASSWORD, session_factory=SessionFactory) as ftp_host:
        # Build full paths
        SD_OUT = ftp_host.path.join(FTP_REMOTE_DIR, PROGRAM_ID)
        OUT = os.path.join(FTP_LOCAL_DIR, PROGRAM_ID)

        # Make output directories
        ftp_host.makedirs(SD_OUT, exist_ok=True)
        ftp_host.makedirs(os.path.join(SD_OUT, 'flags'), exist_ok=True)
        
        # Iterate every file in the deploy directory.
        for name in os.listdir(OUT):
            # Ignore directories, for now.
            if not os.path.isfile(os.path.join(OUT, name)):
                continue
                
            # Upload file to server.
            ftp_host.upload(os.path.join(OUT, name), ftp_host.path.join(SD_OUT, name))

        # Iterate flags
        for name in os.listdir(os.path.join(OUT, 'flags')):
            # Ignore directories, for now.
            if not os.path.isfile(os.path.join(OUT, 'flags', name)):
                continue
                
            # Upload file to server.
            ftp_host.upload(os.path.join(OUT, 'flags', name), ftp_host.path.join(SD_OUT, 'flags', name))
        
        # If local log file exists
        LOG_FILE = os.path.join(FTP_LOCAL_DIR, 'stdio.log')
        if os.path.exists(LOG_FILE):
            # Ensure remote log directory and file
            HOST_LOG_FILE = ftp_host.path.join(FTP_REMOTE_LOG, 'stdio.log')
            if not ftp_host.path.exists(HOST_LOG_FILE):
                ftp_host.makedirs(FTP_REMOTE_LOG, exist_ok=True)
                ftp_host.upload(LOG_FILE, HOST_LOG_FILE)

if __name__ == '__main__':
    main()
