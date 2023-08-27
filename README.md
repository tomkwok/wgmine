<br />
<p align="center">
  <img src=".img/logo.png" alt="WireGuard logo with 'like a boss' pixellated sunglasses" width="80" height="80">

  <h3 align="center">wgmine</h3>

  <p align="center">
    Generate Vanity Public Keys for WireGuard
    <br>
    You know what opening a wireguard config and finding out comments have been stripped feels like.<br>
    You feel lost: who is this? Who is this peer?? Who's got 10.100.100.4?!
    <br>
    <a href="#usage"><strong>Try it out »</strong></a>
    <br>
  </p>
</p>

<!-- TABLE OF CONTENTS -->
<details open="open">
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#about-the-project">About The Project</a>
    </li>
    <li>
      <a href="#faqs">FAQs</a>
      <ul>
        <li><a href="#what-does-vanity-key-mean">What does "Vanity Key" mean?</a></li>
        <li><a href="#does-this-make-the-key-less-secure">Does this make the key less secure?</a></li>
      </ul>
    </li>
    <li>
      <a href="#benchmarks">Benchmarks</a>
    </li>
    <li>
      <a href="#usage">Usage</a>
      <ul>
        <li><a href="#docker">Docker</a></li>
        <li><a href="#binary">Binary</a></li>
      </ul>
    </li>
    <li>
      <a href="#building">Building</a>
      <ul>
        <li><a href="#building-from-source">Building from Source</a></li>
        <li><a href="#building-with-docker">Building with Docker</a></li>
      </ul>
    </li>
    <li><a href="#contributing">Contributing</a></li>
    <li><a href="#license">License</a></li>
  </ol>
</details>



## About The Project

This project lets you generate Wireguard key pairs such that the public key starts
with some prefix you want.

This makes identifying peers in Wireguard configurations much easier, and also
gives the owner of the key style points. 

There's also a poorely written and very slow python script inside the `py` folder if you dare tickling that beast.

This is basically the same as vanity onion urls for Tor network sites. And that's what has inspired this project. 


## FAQs

### What does "Vanity Key" mean?

You get a public key with a customized short prefix (3-5 base64 characters is a sensible size), such as a number or a location.

For example:

```
# locations
NY+p+6jAY4fx5Zs3UMZw6CpyKoNqe+bpk0DASr4NNGQ=
LA+M0ZzicfLygleHA/G/hFiM/Ax0LH+0T2ANN0rirg0=
RM+aK/ipMDTpTAvFINAIs1DThIa6Paz4NzP/pxv3TH4=
MI+T70PpLZMQ3iXx335J4aZlvoNAECgVmy/DUcE+OxU=

# numbers
01+LdFHVkyhqn5WbSgPel9aH9sUCvfexq7t2fTdI+Rc=
02+08YcL5GB44JSPTZOB0jH2vEgcDfodd8zbDZBY71s=
10+/kYE0rlIeZgAXejrYv+f3+psuhHCtDoNnXW7sYiY=
20+f5BbHHGlHfGf7PsEjdXBIzrTOTppwRRQF66mP2n8=

# names
RMN+6VsbTrnltAOhXSz+uvlSoqc4ZKcpLzbamDfVy2k=
TXT+0Xt+xwahPfvNx8hen4mzrSco00dazvIsYq4PRyE=
XXX+d+RvoLrDoYDx9G97oKnJLBYZ12ahHUnhVUtsewE=
SRV+nkiRDYk6Wy2StgUVVqFjjuaadHfbxg+IFQ/+5EY=
007+hWCFxVcCzXruhvA6JnqqzpDQXPpPAcHKau4Xx0A=
```

You don't need to, but I personally like to add `+` at the end of the prefix to make it easier to see.
That's just a preference tho :)

Since public keys are irreversible (by definition) and they depend fully on their private counterpart, the only solution
to the problem is to bruteforce a ton of key pairs until one with a public key matching the prefix is found.


### Does this make the key less secure?

No.

Having a customized prefix on your public keys won't affect its security because
it doesn't give a potential attacker any extra information.

To crack a public key you need to generate private keys until you find one
matching it. An attacker would be looking for the **same** public key, and the prefix
won't thus make any difference. They won't be able to "filter out" keys from this bruteforce
based on the known prefix because they will already look for the whole key. 

Side note: that would be a different story for private keys. There's no such thing
as a "vanity private key" for obvious reasons, but also doing that would lower its security.
It's also way easier to do: you just do it, you can freely choose private keys, no need to mine them.
But please don't. Just let the RNG gods do that job for ya'.


## Benchmarks (AArch64 NEON)

Tests are done on Ampere A1 (4 cores).

Multiprocessing (autodetected): 4

#### Before optimization (with libsodium)

```
Benchmark result: 82431 keys/second
```

#### After optimization (with AArch64 NEON assembly implementation)

```
Benchmark result: 94056 keys/second
```


## Usage

```
wgmine [-b] <PREFIX> [..PREFIXES]
```


### Binary

[Build the project first](#building)

#### Run benchmark

Run without arguments:

```
$ wgmine
```

#### Generate a single key pair

Run with the prefix you want:

```
$ wgmine NYC+
```

#### Bulk generate key pairs

Set the `-b` flag (must be first argument) then specify as many prefixes as you want.
The more the marrier!

```
$ wgmine -b NYC+ LA [...] 
```

#### Output to file

Generated key pairs are printed on standard output. You can just redirect it to a file, like this:

```
$ wgmine NYC+ LA LOL > output.txt
```

**Note:** Warnings and info messages will be printed to standard error, so you 
will still see them in terminal but not in the output file.


### Python Alternative

> **WARNING:** this script is bad and could hurt your feelings. Handle with caution. It's also very slow. 

Don't use this script unless you absolutely need to, it really is slow.

```
$ cd py
$ pip install --user -r requirements.txt
$ python3 wgmine.py
```

I think you should also use a virtualenv. Be careful with the cryptography version.
There are many later broken versions, or maybe the one I use is, but still it can stop 
generating good keys for completely trash ones.

Always verify the generated private keys are valid by comparing their public counterpart
with the output of `wg pubkey`.


## Building


### Building from Source

Install build dependencies:

```
$ sudo apt install gcc make cmake libbsd-dev
$ sudo yum install gcc make cmake libbsd-devel
```

Clone this repo and cd into it:

```
$ git clone https://github.com/tomkwok/wgmine.git
$ git submodule update --init
$ cd wgmine
```

Build the project:

```
$ cmake src
$ make
```

The built (standalone) executable is ``wgmine``. You can install it system-wide by copying it into
any directory in your ``PATH``, such as (usually) `/usr/local/bin`.


### Building with Docker

To build it yourself, clone the repo and then:

```
$ docker build -t wgmine .
```
