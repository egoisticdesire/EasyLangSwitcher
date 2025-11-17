# EasyLangSwitcher

**EasyLangSwitcher** is a simple and lightweight Windows application that allows you to switch keyboard layouts using
**a single key**. Switching cycles through all languages added in the system:

- **Short press** — changes the current layout.
- **Long press** or using **a combination** (for example, **Ctrl+C**) with the assigned key preserves the standard
  behavior of that key.

Currently, the application allows you to use the Left Ctrl key to switch layouts, without using the usual **Alt+Shift**
or **Win+Space** combinations.

The application starts in the system tray and provides basic actions:

- **Left Click** — toggle on/off (enabled by default)
- **Right Click** — open the menu (includes information about current configuration)

_Please note that the application does not work in UAC windows.  
Additionally, some modern Windows 11 applications (for example, the updated Settings app and Calculator) may not respond
to layout switching. The reasons for this are currently unknown, and attempts to make it work have been unsuccessful._

---

To add the application to **Startup**, place a shortcut in:

```
%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup
```

**The idea behind the app** came from personal experience: a dedicated key on the **MacBook** and features of
**BetterTouchTool** inspired the creation of this solution for **Windows**.

## License

The application is distributed for free under the **MIT license**.

## Support

If you find this project useful, you can support its further development via cryptocurrency donations:

**TRX (Tron)**

- **Wallet** (_via exchange_):  
  Address: `TGjD8pobrz8XZbq4KSgTs48fYu5oGX3J8L`
- **Personal wallet**:  
  Address: `TAb46n1FtVBYXmonAtiLbLj61tLhjonWPB`

**TON (Ton Coin)**

- **Wallet** (_via exchange_):  
  Address: `EQBVXzBT4lcTA3S7gxrg4hnl5fnsDKj4oNEzNp09aQxkwj1f`  
  Tag: `487819`
- **Personal wallet**:  
  Address: `UQBJZgPeDNXxnTRomijqE8eo0FuTRcsQzQ6Is-UlSfnSQvgx`
