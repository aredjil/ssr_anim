# Sample Space Reducing Process Animation

This repo contains my implmenetation for the SSR process, currently it supports the standard SSR $\mu = 1$ and SSR cascades $\mu \neq 1$. check my other repo for an overview of the process [SSR](https://github.com/aredjil/ssr). 

## Requirements 

```
c++ compiler 
box2D
SFML
```

## Compile 

To compile the code 

```bash
cmake -S . -B builds 
cmake --build build 
```

## Run the Animation 

To run the animation 

```bash 

./build/ssr_animation.x 
```

## Options 

- `Ctrl + +` Increase the animation speed  
- `Ctrl + -` Decrease the animation speed 
- `Ctrl + f` Edit the multiplicative factor
- `s` Take a screenshot of the current frame
- ` space` Pause the animation
- `r` Restart the animation