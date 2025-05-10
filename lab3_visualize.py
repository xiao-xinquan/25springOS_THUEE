import matplotlib.pyplot as plt
import matplotlib.patches as patches
import csv
import os
from PIL import Image

MEMORY_SIZE = 1024
IMAGE_FOLDER = "step_images"
OUTPUT_IMAGE = "lab3_output.jpg"

def load_memory_steps(filename):
    steps = []
    with open(filename, 'r') as f:
        reader = csv.reader(f)
        for row in reader:
            step_info = int(row[0])
            block_data = row[1:]
            blocks = []
            for i in range(0, len(block_data), 3):
                start = int(block_data[i])
                size = int(block_data[i + 1])
                is_free = int(block_data[i + 2])
                if start == -1:
                    blocks = None
                    break
                blocks.append((start, size, is_free))
            steps.append((step_info, blocks))
    return steps

def render_step(step_id, blocks, save_path):
    fig, ax = plt.subplots(figsize=(10, 1.5))
    ax.set_xlim(0, MEMORY_SIZE)
    ax.set_ylim(0, 1)
    ax.axis('off')
    ax.set_title(f"Step {step_id}", fontsize=10)

    if blocks is None:
        ax.text(10, 0.5, f"Step {step_id}: Allocation Failed", color='red', fontsize=12)
    else:
        for block in blocks:
            start, size, is_free = block
            color = '#A8E6CF' if is_free else '#4B7BE5'
            rect = patches.Rectangle((start, 0), size, 1, edgecolor='black', facecolor=color, linewidth=1)
            ax.add_patch(rect)
            label = f"{start}-{start + size}"
            ax.text(start + size / 2, 0.5, label, ha='center', va='center', color='black', fontsize=8)

    plt.tight_layout()
    plt.savefig(save_path)
    plt.close()

def concat_images_vertically(image_folder, output_file):
    image_files = sorted([f for f in os.listdir(image_folder) if f.endswith('.png')])
    images = [Image.open(os.path.join(image_folder, f)) for f in image_files]

    widths, heights = zip(*(img.size for img in images))
    total_height = sum(heights)
    max_width = max(widths)

    concat_img = Image.new('RGB', (max_width, total_height), color=(255, 255, 255))

    y_offset = 0
    for img in images:
        concat_img.paste(img, (0, y_offset))
        y_offset += img.size[1]

    concat_img.save(output_file)

def main():
    os.makedirs(IMAGE_FOLDER, exist_ok=True)
    steps = load_memory_steps("lab3.csv")

    for i, (step_id, blocks) in enumerate(steps):
        img_path = os.path.join(IMAGE_FOLDER, f"step_{i:03d}.png")
        render_step(step_id, blocks, img_path)

    concat_images_vertically(IMAGE_FOLDER, OUTPUT_IMAGE)
    print(f"visualization successful：{OUTPUT_IMAGE}")

main()
