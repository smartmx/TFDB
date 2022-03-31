from building import *
import rtconfig

cwd = GetCurrentDir()

# init src
src = []
src += Glob('*.c')

CPPPATH = [cwd]

group = DefineGroup('tfdb', src, depend = ['PKG_USING_TFDB'], CPPPATH = CPPPATH)

Return('group')
