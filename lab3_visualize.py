import matplotlib.pyplot as plt
import matplotlib.patches as patches
import matplotlib.animation as animation
import csv

MEMORY_SIZE = 1024
STEP_INTERVAL_MS = 1000

# 读取 CSV 数据
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

# 初始化图像
fig, ax = plt.subplots(figsize=(10, 2))
plt.title("Dynamic Memory Allocation Visualization")
ax.set_xlim(0, MEMORY_SIZE)
ax.set_ylim(0, 1)
ax.axis('off')

rects = []
text_labels = []

steps = load_memory_steps("lab3.csv")

# 更新每一帧
def update(frame):
    global rects, text_labels
    for r in rects:
        r.remove()
    for t in text_labels:
        t.remove()
    rects.clear()
    text_labels.clear()

    step_id, blocks = steps[frame]
    if blocks is None:
        txt = ax.text(10, 0.5, f"Step {step_id}: Allocation Failed", color='red', fontsize=12)
        text_labels.append(txt)
        return

    for block in blocks:
        start, size, is_free = block
        color = '#A8E6CF' if is_free else '#4B7BE5'
        rect = patches.Rectangle((start, 0), size, 1, edgecolor='black', facecolor=color, linewidth=1)
        ax.add_patch(rect)
        rects.append(rect)

        label = f"{start}-{start + size}"
        txt = ax.text(start + size / 2, 0.5, label, ha='center', va='center', color='black', fontsize=8)
        text_labels.append(txt)

ani = animation.FuncAnimation(fig, update, frames=len(steps), interval=STEP_INTERVAL_MS, repeat=False)

# 保存最后一帧为 jpg
def save_final_frame():
    update(len(steps) - 1)
    plt.savefig("output.jpg")

save_final_frame()
plt.show()