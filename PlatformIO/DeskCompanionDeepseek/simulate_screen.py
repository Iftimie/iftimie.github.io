import pygame
import sys
import time

# ===== OLED CONFIG =====
OLED_W = 128
OLED_H = 64
SCALE = 4  # window scale (not text scale)

WIN_W = OLED_W * SCALE
WIN_H = OLED_H * SCALE

WHITE = (255, 255, 255)
BLACK = (0, 0, 0)

# ===== INIT =====
pygame.init()
screen = pygame.display.set_mode((WIN_W, WIN_H))
pygame.display.set_caption("Wordly — Virtual OLED")

clock = pygame.time.Clock()

# IMPORTANT: disable smooth scaling
pygame.transform.set_smoothscale_backend("GENERIC")

# Base font (small, crisp)
font = pygame.font.SysFont("monospace", 10, bold=True)

def draw_wordly(zoom):
    screen.fill(BLACK)

    # Render text once at base resolution
    text = font.render("Wordly", True, WHITE)

    # Nearest-neighbor scaling (CRISP)
    w, h = text.get_size()
    text_zoomed = pygame.transform.scale(
        text,
        (w * zoom, h * zoom)
    )

    # Center in OLED space
    x = (OLED_W - text_zoomed.get_width()) // 2
    y = (OLED_H - text_zoomed.get_height()) // 2

    # Draw scaled to window
    screen.blit(
        pygame.transform.scale(
            text_zoomed,
            (text_zoomed.get_width() * SCALE,
             text_zoomed.get_height() * SCALE)
        ),
        (x * SCALE, y * SCALE)
    )

    pygame.display.flip()

def wait(seconds):
    start = time.time()
    while time.time() - start < seconds:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                sys.exit()
        clock.tick(60)

# ===== ANIMATION =====

# Zoom levels (3 steps)
zoom_levels = [1, 2, 3]

for z in zoom_levels:
    draw_wordly(z)
    wait(0.6)

# Hold final state
while True:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            pygame.quit()
            sys.exit()
    clock.tick(30)
