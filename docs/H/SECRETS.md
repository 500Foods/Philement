# Welcome to Secrets Management in Hydrogen

Hello! If you're new to the Hydrogen project or not very tech-savvy, don't worry. This guide is designed to help you understand and set up the essential security components for Hydrogen in a simple, step-by-step way. We're going to focus on setting up two important pieces: **PAYLOAD_LOCK** and **PAYLOAD_KEY**. These help protect sensitive information in our project. Let's get started!

## What Are Secrets and Why Do They Matter?

In the Hydrogen project, "secrets" are like special keys or passwords that keep sensitive information safe. Instead of writing these secrets directly into our code (which could be risky), we store them in a way that only the right people or systems can access them. This guide will walk you through setting up your secrets so that Hydrogen can work securely.

## Step-by-Step Guide to Setting Up Your Secrets

We're going to create and set up three secrets: **PAYLOAD_LOCK**, **PAYLOAD_KEY**, and **WEBSOCKET_KEY**. Think of PAYLOAD_LOCK as a lock that secures data when we build the project, PAYLOAD_KEY as the key that unlocks it when the project runs, and WEBSOCKET_KEY as a password that protects WebSocket connections. Here's how to do it:

### Step 1: Make Sure You Have the Right Tools

To create these secrets, you'll need a tool called **OpenSSL**. It's like a Swiss Army knife for security tasks. Most computers running Linux or macOS already have it installed. If you're on Windows, you might need to install it (you can download it from a trusted source or use a tool like Git Bash which includes OpenSSL).

To check if you have OpenSSL, open a terminal or command prompt and type:

```bash
openssl version
```

If it shows a version number, you're good to go! If not, ask a friend or search online for "install OpenSSL on [your operating system]."

### Step 2: Create Your Secret Keys

Now, let's create the two keys using OpenSSL. Open your terminal or command prompt and follow these steps carefully. We'll do this in a temporary folder to keep things organized.

1. **Create a temporary folder to store your keys:**

   ```bash
   mkdir temp_keys
   cd temp_keys
   ```

2. **Generate a private key (this is PAYLOAD_KEY):**

   ```bash
   openssl genrsa -out private_key.pem 2048
   ```

   This creates a file called `private_key.pem` which holds your private key. Keep this safe—it's like the key to your house!

3. **Generate a public key (this is PAYLOAD_LOCK):**

   ```bash
   openssl rsa -in private_key.pem -pubout -out public_key.pem
   ```

   This creates a file called `public_key.pem` which holds your public key. This is like the lock that matches your key.

### Step 3: Set Up Your Secrets for Hydrogen

Now that you have your keys, we need to tell Hydrogen where to find them. We do this by setting "environment variables"—think of them as little notes your computer keeps to remember important information.

Here’s how to set them for the current session (they’ll disappear when you restart your computer):

```bash
export PAYLOAD_LOCK=$(cat public_key.pem | base64 -w 0)
export PAYLOAD_KEY=$(cat private_key.pem | base64 -w 0)
```

### INSTALLER KEYS

```bash
openssl genrsa -out HYDROGEN_INSTALLER_KEY.pem 2048  # Private key (keep the key)
openssl rsa -in HYDROGEN_INSTALLER_KEY.pem -pubout -out HYDROGEN_INSTALLER_LOCK.pem  # Public key (share the lock)
export HYDROGEN_INSTALLER_KEY=$(cat HYDROGEN_INSTALLER_KEY.pem | base64 -w 0)
export HYDROGEN_INSTALLER_LOCK=$(cat HYDROGEN_INSTALLER_LOCK.pem | base64 -w 0)
echo $HYDROGEN_INSTALLER_KEY
echo $HYDROGEN_INSTALLER_LOCK

```

What does this do? It reads the contents of your key files, turns them into a format Hydrogen can understand, and stores them in PAYLOAD_LOCK and PAYLOAD_KEY.

If you want these settings to stick around even after restarting your computer, you’ll need to add them to a special file:

- **For Bash users** (common on Linux or Git Bash on Windows), open `~/.bashrc` or `~/.bash_profile` in a text editor and add those two `export` lines.
- **For Zsh users** (common on macOS), open `~/.zshrc` in a text editor and add the lines.

After adding them, run `source ~/.bashrc` (or `~/.zshrc`) to apply the changes.

### Step 4: Set Up Your WebSocket Key

The **WEBSOCKET_KEY** is different from the RSA keys above. It's a simpler password-like secret that protects WebSocket connections. Unlike the RSA keys, you can create this yourself using any secure method.

Here's how to set up a WebSocket key:

1. **Generate a secure random key:**

   ```bash
   openssl rand -hex 32
   ```

   This creates a 64-character hexadecimal string. Copy this value.

2. **Set the environment variable:**

   ```bash
   export WEBSOCKET_KEY="your_generated_key_here"
   ```

   Replace `your_generated_key_here` with the key you generated above.

3. **Make it permanent (optional):**

   Add the export line to your `~/.bashrc` or `~/.zshrc` file just like you did for the payload keys.

**Important WebSocket Key Requirements:**

- Must be at least 8 characters long
- Should contain only printable ASCII characters (no spaces or control characters)
- Should be random and unique to your installation
- Keep it secret - don't share it or commit it to version control

### Step 5: Test That Everything Works

Hydrogen comes with handy tests to make sure your secrets are set up correctly:

**For payload keys (PAYLOAD_LOCK and PAYLOAD_KEY):**

```bash
./tests/test_12_env_payload.sh
```

**For WebSocket key (WEBSOCKET_KEY):**

```bash
./tests/test_36_websockets.sh
```

If everything is set up right, you’ll see a success message. If something’s wrong, it will tell you what needs fixing. Don’t hesitate to ask for help if you’re stuck!

### Step 6: Clean Up

Once you’ve set up your secrets, you don’t need the temporary files anymore. You can delete the `temp_keys` folder to keep things tidy:

```bash
cd ..
rm -rf temp_keys
```

## Keeping Your Secrets Safe

Remember, your PAYLOAD_KEY (the private key) is very important. Don’t share it with anyone or commit it to version control (like Git). If you think it’s been compromised, generate a new pair of keys by following the steps above again.

## For Curious Minds: How Does This Work Behind the Scenes?

If you’re interested in the technical details of how Hydrogen uses these secrets to protect data, we’ve got a section just for you. This part is optional and a bit more advanced, so feel free to skip it if you’re just getting started.

### The Magic of Encryption in Hydrogen

Hydrogen uses a clever system to keep data safe, combining two types of security: **RSA** and **AES**. Here’s a simple breakdown:

- When we build Hydrogen, PAYLOAD_LOCK (the public key) helps lock up sensitive data so no one can peek at it.
- When Hydrogen runs, PAYLOAD_KEY (the private key) unlocks that data so the program can use it.

Here’s a more detailed look at the process:

1. **During Build Time:**
   - A random secret code (AES key) is created just for this build.
   - The data we want to protect (like a web interface) is squeezed down small (compressed) and then locked with this AES key.
   - The AES key itself is locked with PAYLOAD_LOCK, so even it is protected.
   - Everything is bundled together and added to the Hydrogen program.

2. **During Run Time:**
   - Hydrogen finds the locked data inside itself.
   - It uses PAYLOAD_KEY to unlock the AES key.
   - With the AES key, it unlocks the actual data, expands it, and uses it.

This double-locking system means that even if someone gets a piece of the puzzle, they can’t see the whole picture without PAYLOAD_KEY.

### Technical Specs (For Experts)

- **AES Details**: Uses AES-256-CBC, a strong encryption method with a random 256-bit key and a 16-byte random start value (IV). It’s like a super-secure safe.
- **RSA Details**: Uses 2048-bit keys with PKCS1 padding, a trusted way to protect smaller pieces of data like the AES key.
- **Compression**: Data is squeezed with Brotli at maximum quality to make it smaller before locking.

## What’s Next for Secrets in Hydrogen?

As Hydrogen grows, we’ll add more secrets for different features, like authentication for a terminal system or secure logins with OpenID Connect. They’ll all follow the same idea of using environment variables to keep things safe.

## Need More Help or Info?

- Check out [README.md](/docs/H/README.md) for general info on Hydrogen.
- Look at [payloads/README.md](/elements/001-hydrogen/hydrogen/payloads/README.md) for more on the payload system.
- Visit the [OpenSSL Documentation](https://www.openssl.org/docs/) if you’re curious about the tools we used.

If you have questions or run into trouble, don’t hesitate to ask someone on the team or look for help online. We’re all in this together!
