# Light Stick Control System

This project is a Wi-Fi-controlled Light Stick based on the ESP32-C3 SuperMini development board. Users can connect via smartphone to a web-based console to instantly change colors, visual effects, brightness, and animation speeds for the top and bottom sections of the light stick. It specifically includes POV effects designed for spinning and juggling.

## 🗝️ How To Use It

When you turn on the power of the light stick, it will run six colors in default, which include red, yellow, green, blue, purple, white. It will repeat again and again until you change it.

So, how to control it? It is easy! To control it, you need a smartphone. When you turn on your light stick, a wifi named "My_Light_Stick" will pop up on the wifi searching page. (The password is "88888888") Connect to it, and enter IP "192.168.4.1" in your browser's address bar. Wait a second, you will see a page like this:

![control default picture](https://github.com/user-attachments/assets/1023e112-8935-48c1-abc2-9bf26da10033)

You can see three buttons on the top of the web called "SYNC", "TOP", and "BTM", which means you can control it in three methods.
Then, under the three buttons, you will see another two buttons called "Static/Color-Jumping" and "Special Effects Library".When you choose "Static/Color-Jumping" button, you can see both "Palette" and "Brightness Control Bar". Whatever color you choose from the palette, that will be the color of the light stick. There are also some default colors for you to choose.

![static page picture](https://github.com/user-attachments/assets/c1d728ab-4031-4dfa-8ea1-d7e52144bc33)

Next, let's focus on the "Special Effect Library" button. When you click the button, a drop-down menu will appear. Click the drop-down menu, you will see some effects you can pick.

![special effect library picture](https://github.com/user-attachments/assets/493e887d-6da4-4e40-b602-9a86634741b8)

Also, take a look at the more detailed information which located at the end.

And now, power it up, connect your phone to the light stick's Wi-Fi, and start spinning!
by young

## 🛠️ Hardware Configuration

* **Microcontroller**: ESP32-C3 SuperMini
* **LED Type**: WS2812B
* **Structure & LED Layout**:
  * **Total Strips**: 4 strips
  * **Dual-Head Design**: Split into "TOP" and "BTM" sections.

## ⚙️ Network & Connection Setup

Upon powering up, the ESP32 automatically creates a Wi-Fi hotspot (AP mode). Use your smartphone or computer to connect and control the stick:

* **Wi-Fi SSID**: `My_Light_Stick`
* **Wi-Fi Password**: `88888888`
* **Console URL**: Open a web browser and go to `http://192.168.4.1` (Default ESP32 AP IP)

## 🎨 Built-in Effects Library

The console allows you to apply effects to both heads ("SYNC"), just the "TOP", or just the "BTM". The built-in effects include:

### Basic & Static

* 🔵 **Static/Color Jump**: Manually select a solid RGB color.
* 🔄 **6-Color Cycle (Slow)**: Automatically cycles through Red, Yellow, Green, Blue, Purple, and White.
* 🫁 **Monochrome Breathe**: Smoothly fades the selected color in and out.

### Dynamic & Flow

* 🌈 **Rainbow Gradient (Smooth)**: Classic flowing rainbow effect.
* ⚡ **7-Color Flash (Jump)**: Rapidly cycles through highly saturated hues, perfect for fast beats.
* 🚓 **Police**: High-speed alternating red and blue flashes.
* 🔥 **Fire**: Simulates flickering flames with random red and yellow hues.
* 🌈 **Hyper Rainbow**: High-speed, dense rainbow gradient designed for wide swinging motions.

### 🌀 POV (Persistence of Vision) Effects

* 🌀 **Strobe (Dashed Halo)**: Uses high-frequency strobing (mostly off, briefly white) to draw dashed circles in the air when spun.
* 💈 **Segment (Red/White)**: Alternates red and white rapidly, creating a striped ring effect during rotation.
* ✨ **Fractal Stardust (Dot)**: Randomly lights up individual LEDs, creating a trailing starfield effect.

## ⚠️ Important Notes

* **Wi-Fi Range & Connection:** To ensure the light stick runs smoothly on battery power, its Wi-Fi signal strength is intentionally optimized for stability rather than extreme distance. For the best experience, keep your phone relatively close (within a few meters) to the light stick when changing effects.
* **Low Battery Symptoms:** These high-intensity LEDs consume a lot of power, especially on bright white or solid color settings. If you notice the Wi-Fi disconnecting frequently, the control panel becoming unresponsive, or the lights flickering abnormally, it usually means the battery is running low and needs a recharge!
* **Heat Generation:** Running the light stick on maximum brightness for extended periods can cause the LEDs to get warm to the touch. This is completely normal. To save battery and keep the stick cool, you can easily lower the brightness using the web console.
* **Disconnection:** Once you have set your favorite effect, you can safely disconnect your phone's Wi-Fi or close the browser. The light stick will continue running the last selected effect until you turn it off.

## 🙋‍♀️ Frequently Asked Questions (Q&A)

**Q1: I can't access the control panel at `192.168.4.1`. What should I do?**
**A:** First, double-check that your phone is currently connected to the `My_Light_Stick` Wi-Fi network. Since this network does not provide internet access, some smartphones might aggressively try to disconnect you. If you can't load the page, please try these steps:

1. Temporarily turn off your mobile data (4G/5G).
2. Ensure that any **"Dual-Band Wi-Fi"**, "Wi-Fi Assistant", or "Smart Network Switching" features in your phone's Wi-Fi settings are **turned off**.
3. Reconnect to the stick's Wi-Fi and refresh the browser page.

**Q2: The lights are flickering randomly, or the Wi-Fi keeps dropping!**
**A:** This is the most common sign of a low battery! High-brightness effects (especially pure white) draw a lot of power. When the battery gets too low, the system struggles to keep the Wi-Fi and lights running simultaneously. Please recharge your light stick.

**Q3: Do I need to keep my phone connected to the stick while I'm spinning it?**
**A:** Not at all! Once you select your desired color or effect on the web panel, the light stick will remember it and keep running. You can safely disconnect from the Wi-Fi, lock your phone, and put it in your pocket.

**Q4: How do the "SYNC", "TOP", and "BTM" buttons work?**
**A:** These buttons let you control different zones of the stick:

* **SYNC:** Applies your chosen color or effect to the *entire* stick at the same time.
* **TOP:** Changes only the upper half of the stick.
* **BTM:** Changes only the lower half of the stick.
* *Tip: You can set a breathing effect on the TOP and a static color on the BTM for a cool mixed look!*

**Q5: The light stick feels warm to the touch after a while. Is this safe?**
**A:** Yes, this is completely normal. The LEDs generate heat, especially when running at maximum brightness for a long time. If it gets too warm, you can easily lower the brightness using the brightness slider (☀) on the control panel to cool it down and save battery.

## 📩 Contact & Customization

Feel free to reach out if you have any questions, run into issues, or if you're interested in customized features for your light stick!

* **Email:**  <liyoung74788@gmail.com>
