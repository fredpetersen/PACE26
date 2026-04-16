import sys

x,y = map(int, sys.argv[1:3])

def cantor_pairing(x,y):
    return (x+y)*(x+y+1)//2 + y

print(cantor_pairing(x,y))