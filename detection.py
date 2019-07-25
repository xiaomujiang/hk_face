# -*- coding:utf-8 -*-
import time
import ctypes
import os
from conf import *


def upload_face_server(camera_ip, name, face_id, face_train, username, password, sdk_path):
    '''
    Upload face to hk face camera using sdk
    :param camera_ip: face camera ip
    :param name: face name
    :param face_id: face id(eg:00000001)
    :param face_train: face image path
    :param password: face camera password
    :return: res(1 is ok)
    '''
    # sex is male or female
    sex = "男"
    os.chdir(sdk_path)
    if sex == "男":
        sex = "male"
    else:
        sex = "female"

    # XML to read face info
    hk_xml = os.path.join(sdk_path, "tmp.xml")
    f = open(hk_xml, 'w')
    f.writelines("<?xml version=\'1.0\' encoding=\'UTF-8\'?>\n")
    f.writelines("<FaceAppendData>\n")
    f.writelines("<name>" + name + "</name>\n")
    f.writelines("<sex>" + sex + "</sex>\n")
    f.writelines("<certificateType>ID</certificateType><certificateNumber>" + face_id + "</certificateNumber>\n")
    f.writelines("</FaceAppendData>\n")
    f.close()

    hk_dll = os.path.join(sdk_path, "getpsdata.so")
    # python call c++ using ctypes
    h_dll = ctypes.cdll.LoadLibrary(hk_dll)
    # hk sdk input data must be bytes
    camera_ip = bytes(camera_ip, 'ascii')
    password = bytes(password, 'ascii')
    username = bytes(username, 'ascii')
    face_train = bytes(face_train, 'ascii')
    sdk_path = bytes(sdk_path, 'ascii')
    # UploadFile
    '''
    param camera_ip:hk face camera
    param port: default 8000
    param user_name:user_name
    param user_pwd:user_pwd
    param face_train: face image path
    '''
    res = h_dll.UploadFile(camera_ip, 8000, username, password, face_train, sdk_path)

    return res


def face_match_snap(camera_ip, sdk_path, camera_name, camera_pwd, img_save_path, camera_port=8000):
    face_snap_so = os.path.join(sdk_path, "getpsdata.so")
    os.chdir(sdk_path)
    h_dll = ctypes.cdll.LoadLibrary(face_snap_so)
    # hk sdk input data must be bytes
    _camera_ip = bytes(camera_ip, 'ascii')
    _camera_name = bytes(camera_name, 'ascii')
    _camera_pwd = bytes(camera_pwd, 'ascii')
    _img_save_path = bytes(img_save_path, 'ascii')
    # FaceDetectAndContrast
    '''
    param camera_ip:hk face camera
    param port: default 8000
    param user_name:user_name
    param user_pwd:user_pwd
    '''
    if not os.path.exists(IMG_SAVE_PATH):
        os.makedirs(IMG_SAVE_PATH)
    h_dll.FaceDetectAndContrast(_camera_ip, camera_port, _camera_name, _camera_pwd, _img_save_path)
    while True:
        # keep alive
        time.sleep(200)


if __name__ == '__main__':
    face_match_snap(CAMERA_IP, SDK_PATH, CAMERA_NAME, CAMERA_PWD, IMG_SAVE_PATH)
