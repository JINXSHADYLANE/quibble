import pyglet
import random

langas = pyglet.window.Window(320, 640)
fonas = pyglet.resource.image('fonas.png')
plyta = pyglet.resource.image('plyta.png')

plytos_ilgis = 32
plytos_plotis = 32

blokai = [
	[(0, 1), (1, 1), (1, 0), (2, 0)],
	[(0, 0), (1, 0), (1, 1), (2, 1)],
	[(0, 0), (1, 0), (2, 0), (1, 1)],
	[(0, 1), (0, 0), (1, 0), (2, 0)],
	[(0, 0), (1, 0), (2, 0), (2, 1)],
	[(0, 0), (1, 0), (0, 1), (1, 1)],
	[(0, 0), (1, 0), (2, 0), (3, 0)]
]

sulinio_plotis = 10
sulinio_aukstis = 20
sulinys = [[0] * sulinio_plotis for _ in range(sulinio_aukstis)]

dangus = (4, 10)

dabartinis_blokas = 0
bloko_vieta = (0, 0)
bloko_kampas = 0

def naujas_blokas():
	global dabartinis_blokas, bloko_vieta, bloko_kampas
	dabartinis_blokas = random.randint(0, 6)
	bloko_vieta = dangus
	bloko_kampas = 0

def piesti_bloka(blokas):
	for p in blokas:
		plyta.blit(plytos_plotis * p[0], plytos_ilgis * p[1])	

def piesti_sulini(sulinys):
	for y, eilute in enumerate(sulinys):
		for x, langelis in enumerate(eilute):
			if langelis == 1:
				plyta.blit(x * plytos_plotis, y * plytos_ilgis)

def sukti_bloka(blokas, kampas):
	if kampas % 4 == 0:
		return blokas
	return sukti_bloka([(1-y, x) for (x, y) in blokas], kampas - 1)	

def slinkti_bloka(blokas, poslinkis):
	return [(x + poslinkis[0], y + poslinkis[1]) for (x, y) in blokas]

def judinti_bloka(blokas, kampas, poslinkis):
	return slinkti_bloka(sukti_bloka(blokas, kampas), poslinkis)

def ar_telpa(sulinys, blokas):
	for p in blokas:
		if p[1] >= sulinio_aukstis:
			continue
		if p[1] < 0 or p[0] < 0 or p[0] >= sulinio_plotis:
			return False
		if sulinys[p[1]][p[0]] == 1:
			return False
	return True		

def ispausti(sulinys, blokas):
	for p in blokas:
		sulinys[p[1]][p[0]] = 1

def trinti_eilutes():
	global sulinys
	i = 0
	while i < len(sulinys):
		if sulinys[i] == [1] * sulinio_plotis:
			sulinys = sulinys[:i] + sulinys[i+1:] + [[0] * sulinio_plotis]
		else:
			i += 1

def kristi(dt):
	global bloko_vieta
	nauja_vieta = (bloko_vieta[0], bloko_vieta[1] - 1)
	if ar_telpa(sulinys, judinti_bloka(
		blokai[dabartinis_blokas], bloko_kampas, nauja_vieta)):
			bloko_vieta = nauja_vieta
	else:
		ispausti(sulinys, judinti_bloka(
			blokai[dabartinis_blokas], bloko_kampas, bloko_vieta))
		trinti_eilutes()	
		naujas_blokas()	
			
pyglet.clock.schedule_interval(kristi, 0.4)	

@langas.event
def on_draw():
	langas.clear()
	fonas.blit(0, 0)
	piesti_bloka(judinti_bloka(blokai[dabartinis_blokas],
		bloko_kampas, bloko_vieta))
	piesti_sulini(sulinys)

@langas.event	
def on_key_press(simbolis, _):
	global bloko_vieta, bloko_kampas
	nauja_vieta = bloko_vieta
	naujas_kampas = bloko_kampas
	if simbolis == pyglet.window.key.LEFT:
		nauja_vieta = (bloko_vieta[0] - 1, bloko_vieta[1])
	if simbolis == pyglet.window.key.RIGHT:
		nauja_vieta = (bloko_vieta[0] + 1, bloko_vieta[1])
	if simbolis == pyglet.window.key.UP:
		naujas_kampas += 1
	if simbolis == pyglet.window.key.DOWN:
		nauja_vieta = (bloko_vieta[0], bloko_vieta[1] - 1)
		while ar_telpa(sulinys, judinti_bloka(
			blokai[dabartinis_blokas], bloko_kampas, nauja_vieta)):
			bloko_vieta = nauja_vieta
			nauja_vieta = (bloko_vieta[0], bloko_vieta[1] - 1)

	if ar_telpa(sulinys, judinti_bloka(
		blokai[dabartinis_blokas], naujas_kampas, nauja_vieta)):
		bloko_vieta = nauja_vieta
		bloko_kampas = naujas_kampas

naujas_blokas()
pyglet.app.run()	
