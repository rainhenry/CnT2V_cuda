#!/usr/bin/python3
#*************************************************************
#
#   程序名称:Python模块子程序
#   程序版本:REV 0.1
#   创建日期:20240729
#   设计编写:王祥福
#   作者邮箱:rainhenry@savelife-tech.com
#
#   版本修订
#       REV 0.1   20240729      王祥福    创建文档
#
#*************************************************************
##  导入模块
import os
import gc
from typing import Optional, Union, List, Callable
import diffusers
import transformers
import numpy as np
import IPython
import ipywidgets as widgets
import torch
import PIL
import gradio as gr
from pathlib import Path
import cv2
from diffusers.utils.torch_utils import randn_tensor
from diffusers.utils import export_to_video

##  导出视频为MP4文件
def export_to_mp4(video_frames: List[PIL.Image.Image], output_video_path: str = None, fps: int = 8) -> str:
    gc.disable()   ##  关闭垃圾回收
    if output_video_path is None:
        return None

    video_frames = [np.array(frame) for frame in video_frames]

    fourcc = cv2.VideoWriter_fourcc(*"mp4v")
    h, w, c = video_frames[0].shape
    video_writer = cv2.VideoWriter(output_video_path, fourcc, fps=fps, frameSize=(w, h))
    for i in range(len(video_frames)):
        img = video_frames[i]
        img = np.round(img)
        img = img.astype(np.uint8)
        np.clip(img, 0, 255, out=img)
        img[:,:,[0,1,2]] = img[:,:,[2,1,0]]
        video_writer.write(img)
    return output_video_path

##  获取原始模型的管道
def get_origin_model_pipe(model_id):
    gc.disable()   ##  关闭垃圾回收
    pipe = diffusers.DiffusionPipeline.from_pretrained(model_id)
    return pipe

##  获取无加速器模型的管道
def noacc_model_pipeline(pipe):
    ##pipe.scheduler = diffusers.DPMSolverMultistepScheduler.from_config(pipe.scheduler.config)
    pipe = pipe.to('cpu')
    return pipe

##  获取GPU加速器模型的管道
def gpu_model_pipeline(pipe):
    ##pipe.scheduler = diffusers.DPMSolverMultistepScheduler.from_config(pipe.scheduler.config)
    pipe = pipe.to('cuda')
    return pipe

##  推理
def text_to_video(prompt, pipe, steps, in_width, in_height, in_frames, out_gif_file, out_mp4_file, callback=None):
    video_frames = pipe(prompt, num_inference_steps=steps, height=in_height, width=in_width, num_frames=in_frames, callback=callback).frames

    ##  导出视频文件
    export_to_video(video_frames=video_frames[0], output_video_path=out_mp4_file)

    ##  导出动图文件
    video_frames_float32 = np.clip(video_frames[0]*255.0, 0, 255)
    video_frames_uint8 = np.uint8(video_frames_float32)
    images = [PIL.Image.fromarray(frame) for frame in video_frames_uint8]
    images[0].save(out_gif_file, save_all=True, append_images=images[1:], duration=125, loop=0)
    
##  载入翻译模型
def translate_model_init(model_id):
    gc.disable()   ##  关闭垃圾回收
    tokenizer = transformers.AutoTokenizer.from_pretrained(model_id)
    model = transformers.AutoModelForSeq2SeqLM.from_pretrained(model_id)
    ret = [tokenizer, model]
    return ret

##  执行翻译
def translate_cn_to_en(in_text, tsl_model):
    gc.disable()   ##  关闭垃圾回收
    tokenizer = tsl_model[0]
    model = tsl_model[1]
    inputs = tokenizer(in_text, return_tensors='pt')
    pred = model.generate(**inputs)
    output = tokenizer.decode(pred.cpu()[0], skip_special_tokens=True)
    return output
