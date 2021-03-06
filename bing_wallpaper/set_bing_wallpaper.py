#!/usr/bin/python3

# ubuntu设置动态bing壁纸

from PIL import Image, ImageDraw, ImageFont
import requests
import os, re

save_dir = "/home/leon/pic/wallpaper/bing/"

def set_desktop(pic_path):
    cmd = "gsettings set org.gnome.desktop.background picture-uri file:" + pic_path
    # 设置壁纸
    ret = os.system(cmd)          
    if ret != 0:
        print("os.system failed, now exit")
        exit(-1)
    
#获取今天的图片的url和copyright信息
def get_bing_info():
    base_url = "http://cn.bing.com/"
    query_url = "HPImageArchive.aspx?format=js&idx=0&n=1"

    r = requests.get(base_url + query_url)

    image_url = base_url + r.json()["images"][0]["url"]
    copyright = r.json()["images"][0]["copyright"]
    copyright = re.sub(r'\(.*\)', '', copyright);
    print(copyright)

    return (image_url, copyright)

#下载图片
def down_pic(pic_url, save_dir):
    image_name = re.sub(r'.*/', '', pic_url)
    save_path = save_dir + image_name
    print(save_path)

    r = requests.get(pic_url)
    file = open(save_path, "wb")
    file.write(r.content)
    file.close()

    return save_path

#添加水印
def add_watermark(path, text):
    new_path = path + ".watermark"
    cmd = "convert {0} -font 文泉驿微米黑 -pointsize 30 -draw \"gravity south fill black  text 0,72 '{1}'  fill white  text 1,71 '{1}' \" {2}".format(path, text, new_path)

    #print(cmd)
    os.system(cmd)   
    os.system("mv {} {}".format(new_path, path))

#通过python的PIL库添加水印
def add_watermark_by_PIL(path, text):
    img = Image.open(path)
    mode = img.mode
    img_w, img_h = img.size

    font_size = int(img_h / 35)

    if len(mode) == 1:  # L, 1
        white = (255)
        black = (0)
    if len(mode) == 3:  # RGB
        white = (255, 255, 255)
        black = (0,0,0)
    if len(mode) == 4:  # RGBA, CMYK
        white = (255, 255, 255, 255)
        black = (0,0,0,0)

    # 计算text长度，好让文字居中

    # 字体可能要改
    # linux查看支持的汉字字体 # fc-list :lang=zh
    try:
        ft = ImageFont.truetype("DroidSansFallbackFull.ttf", font_size)
    except:
        ft = ImageFont.truetype("wqy-microhei.ttc", font_size)

    font_w, font_h = ft.getsize(text)
    start_px = int((img_w - font_w) / 2)

    #开始添加水印，同时添加黑白水印，处理深色和浅色背景问题
    draw = ImageDraw.Draw(img)
    draw.text((start_px + 2, img_h - font_size - 18), text, black, font=ft)
    draw.text((start_px, img_h - font_size - 20), text, white, font=ft)

    img.save(path)

def main():
    url, copyright = get_bing_info()
    path = down_pic(url, save_dir)
    add_watermark(path, copyright)
    # add_watermark_by_PIL(path, copyright)
    set_desktop(path)

main()
# add_watermark_by_PIL("5.jpg", "卡拉尼石巨石阵，苏格兰路易斯岛")