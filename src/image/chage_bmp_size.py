
import os
from PIL import Image
#将一个目录下的所有bmp格式图片更改为指定尺寸，保存到指定目录
def resize_bmp_images(input_dir, output_dir, target_size):
    # 确保输出目录存在
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    # 遍历输入目录中的所有文件
    for root, dirs, files in os.walk(input_dir):
        for file in files:
            if file.lower().endswith('.bmp'):  # 只处理 BMP 文件
                # 构造完整的输入文件路径
                input_path = os.path.join(root, file)
                
                # 构造输出目录结构
                relative_path = os.path.relpath(root, input_dir)
                output_subdir = os.path.join(output_dir, relative_path)
                if not os.path.exists(output_subdir):
                    os.makedirs(output_subdir)
                
                # 构造完整的输出文件路径
                output_path = os.path.join(output_subdir, file)
                
                # 打开图像并调整大小
                with Image.open(input_path) as img:
                    resized_img = img.resize(target_size, Image.Resampling.LANCZOS)
                    resized_img.save(output_path)
                
                print(f"Resized and saved: {output_path}")

if __name__ == "__main__":
    input_directory = 'game/bullet'  # 输入 BMP 文件的文件夹路径
    output_directory = 'game/bullet1'  # 输出 BMP 文件的文件夹路径
    target_size = (24, 24)  # 指定图像的新尺寸 (宽, 高)

    resize_bmp_images(input_directory, output_directory, target_size)
