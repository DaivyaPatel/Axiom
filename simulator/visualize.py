import numpy as np
import matplotlib.pyplot as plt
import os

# Lorenz parameters
sigma = 10.0
rho = 28.0
beta = 8.0 / 3.0

def lorenz(s):
    x, y, z = s
    return np.array([
        sigma * (y - x),
        x * (rho - z) - y,
        x * y - beta * z
    ])

def rk4_step(s, dt):
    k1 = lorenz(s)
    k2 = lorenz(s + 0.5 * dt * k1)
    k3 = lorenz(s + 0.5 * dt * k2)
    k4 = lorenz(s + dt * k3)
    return s + (dt / 6.0) * (k1 + 2 * k2 + 2 * k3 + k4)

def main():
    print("Running Python Chaos Visualizer...")
    steps = 100000
    dt = 0.0001
    
    s1 = np.array([1.0, 1.0, 1.0])
    s2 = np.array([1.0001, 1.0, 1.0]) # For Lyapunov divergence
    
    # Preallocate arrays
    xs, ys, zs = np.zeros(steps), np.zeros(steps), np.zeros(steps)
    prices = np.zeros(steps)
    rates = np.zeros(steps)
    quantities = np.zeros(steps)
    lyapunov_div = np.zeros(steps)
    
    divergence_sum = 0.0
    
    for i in range(steps):
        s1 = rk4_step(s1, dt)
        s2 = rk4_step(s2, dt)
        
        dx = s1[0] - s2[0]
        dy = s1[1] - s2[1]
        dz = s1[2] - s2[2]
        dist = np.sqrt(dx*dx + dy*dy + dz*dz)
        if dist > 0:
            divergence_sum += np.log(dist / 0.0001)
        
        xs[i], ys[i], zs[i] = s1
        prices[i] = 100.0 + (s1[0] * 0.1)
        
        arrival_rate = int(abs(s1[1]) * 0.5)
        rates[i] = max(1, arrival_rate)
        
        qty = int(abs(s1[2]))
        quantities[i] = max(1, min(100, qty))
        
        lyapunov_div[i] = divergence_sum / ((i + 1) * dt)
    
    # Plotting configuration
    plt.style.use('dark_background')
    fig = plt.figure(figsize=(16, 12))
    
    time_arr = np.arange(steps) * dt
    
    # Accent colors
    cyan = '#00ffff'
    orange = '#ff9900'
    pink_orange = '#ff3366'
    
    # ----------------------------------------------------
    # Panel 1: Price time series
    # ----------------------------------------------------
    ax1 = fig.add_subplot(221)
    ax1.plot(time_arr, prices, color=cyan, linewidth=0.8)
    ax1.set_title("Market Price Engine Fluctuations", color='white', pad=10, fontsize=14)
    ax1.set_xlabel("Simulation Time (s)", color='#aaaaaa')
    ax1.set_ylabel("Synthetic Price", color='#aaaaaa')
    ax1.grid(True, alpha=0.15)
    
    # ----------------------------------------------------
    # Panel 2: 3D Lorenz attractor
    # ----------------------------------------------------
    ax2 = fig.add_subplot(222, projection='3d')
    # Fade alpha down so paths form dense structures beautifully
    ax2.plot(xs, ys, zs, color=orange, linewidth=0.4, alpha=0.6)
    ax2.set_title("3D Lorenz Phase Space", color='white', pad=10, fontsize=14)
    ax2.set_xlabel("X (Price)", color='#aaaaaa')
    ax2.set_ylabel("Y (Arrival Rate)", color='#aaaaaa')
    ax2.set_zlabel("Z (Order Size)", color='#aaaaaa')
    ax2.xaxis.set_pane_color((0.1, 0.1, 0.1, 1.0))
    ax2.yaxis.set_pane_color((0.1, 0.1, 0.1, 1.0))
    ax2.zaxis.set_pane_color((0.1, 0.1, 0.1, 1.0))
    
    # ----------------------------------------------------
    # Panel 3: Order arrival rate
    # ----------------------------------------------------
    ax3 = fig.add_subplot(223)
    ax3.plot(time_arr, rates, color=cyan, linewidth=0.5, alpha=0.8)
    ax3.set_title("Tick/Order Arrival Rate", color='white', pad=10, fontsize=14)
    ax3.set_xlabel("Simulation Time (s)", color='#aaaaaa')
    ax3.set_ylabel("Orders per Tick", color='#aaaaaa')
    ax3.grid(True, alpha=0.15)
    
    # ----------------------------------------------------
    # Panel 4: Lyapunov Divergence
    # ----------------------------------------------------
    ax4 = fig.add_subplot(224)
    ax4.plot(time_arr, lyapunov_div, color=pink_orange, linewidth=1.2)
    ax4.set_title("Lyapunov Trajectory Divergence (Log Scaled)", color='white', pad=10, fontsize=14)
    ax4.set_xlabel("Simulation Time (s)", color='#aaaaaa')
    ax4.set_ylabel("Divergence", color='#aaaaaa')
    ax4.set_yscale('symlog') # Helps prevent 0 / negative infinity math errors from pure log
    ax4.grid(True, alpha=0.15, which='both')
    
    # Layout processing
    fig.tight_layout(pad=3.0)
    
    # Output file
    os.makedirs('simulator', exist_ok=True)
    out_path = os.path.join('simulator', 'chaos_output.png')
    
    plt.savefig(out_path, dpi=150, facecolor=fig.get_facecolor(), edgecolor='none')
    print(f"Successfully generated high-DPI plot at: {out_path}")

if __name__ == "__main__":
    main()
