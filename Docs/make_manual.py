#!/usr/bin/env python3
"""Generates the NF Processor Effects user manuals (English + Portuguese) as PDFs.

Inputs kept in the repo: Docs/manual/img/*.png (captures made with NFPESnapshot, see CLAUDE.md), Source/DSP/Programs.h (factory program table),
Assets/logo_nf_audio_tools_official.png.
Usage:  python3 Docs/make_manual.py   ->  Assets/Manuals/NF_ProcessorEffects_Manual_{English,Portugues}.pdf
The manual text is original (written for this plug-in); regenerate after any UI or DSP change."""
import os, re
from PIL import Image
from reportlab.lib.pagesizes import A4
from reportlab.lib.units import mm
from reportlab.lib.colors import Color, white, HexColor
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.enums import TA_LEFT, TA_CENTER
from reportlab.platypus import (BaseDocTemplate, PageTemplate, Frame, Paragraph, Spacer, Table, TableStyle, Image as RLImage,
                                PageBreak, KeepTogether, NextPageTemplate, Flowable)
from reportlab.platypus.tableofcontents import TableOfContents

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
IMG = os.path.join(ROOT, 'Docs/manual/img')
OUT = os.path.join(ROOT, 'Assets/Manuals')
VERSION = re.search(r'VERSION (\d+\.\d+\.\d+)', open(os.path.join(ROOT, 'CMakeLists.txt')).read()).group(1)
LOGO = os.path.join(ROOT, 'Assets/logo_nf_audio_tools_official.png')
W, H = A4
M = 18 * mm
BLUE = HexColor('#1d4f9f'); DARK = HexColor('#123a7a'); LIGHT = HexColor('#e8f0fc'); CYAN = HexColor('#0b7fb0'); ORANGE = HexColor('#ff6224')
INK = HexColor('#14202e'); GREY = HexColor('#5a6678')

# ------------------------------------------------------------------ data
def read_programs():
    txt = open(os.path.join(ROOT, 'Source/DSP/Programs.h')).read()
    rows = []
    for m in re.finditer(r'\{"((?:[^"\\]|\\.)*)",(\d),"([^"]+)","([^"]+)",(\d),(\d)\}', txt):
        t, app, p1, p2, r, mask = m.groups()
        rows.append((t.replace('\\"', '"'), int(app), p1, p2, int(r), int(mask)))
    assert len(rows) == 99, len(rows)
    return rows
PROGRAMS = read_programs()
def title_case(s):
    out, up = '', True
    for ch in s:
        if ch.isalpha(): out += ch.upper() if up else ch.lower(); up = False
        else: out += ch; up = ch not in "'0123456789"
    return out

APPS = {'en': ['Vocal', 'Live Sound', 'Instrumental', 'Drums & Percussion', 'Special'],
        'pt': ['Vocal', 'Som ao vivo', 'Instrumental', 'Bateria e percussão', 'Especial']}

# effects: (name, io, [(knob, range)], text_en, text_pt)
REVERBS = [
 ('Small Hall', 'Stereo', [('Pre Delay', '0-100 ms'), ('Decay', '0-99'), ('Liveliness', '0-99')],
  'A concert hall in a small size: natural, open, and it stays behind the source without changing it.', 'Um salão de concerto em tamanho pequeno: natural, aberto, e fica atrás da fonte sem alterá-la.'),
 ('Large Hall', 'Stereo', [('Pre Delay', '0-200 ms'), ('Decay', '0-99'), ('Liveliness', '0-99')],
  'A large hall with a long, smooth tail and slowly growing density. The most natural choice for orchestral or big ambient sounds.', 'Um salão grande, com cauda longa e suave e densidade que cresce devagar. A escolha mais natural para sons orquestrais ou ambientes amplos.'),
 ('Small Plate', 'Stereo', [('Pre Delay', '0-100 ms'), ('Decay', '0-99'), ('Liveliness', '0-99')],
  'A bright, dense metal plate with no early reflections. Thickens and brightens the sound; classic for pop music and percussion.', 'Uma placa metálica brilhante e densa, sem reflexões iniciais. Engrossa e ilumina o som; clássica em música pop e percussão.'),
 ('Large Plate', 'Stereo', [('Pre Delay', '0-200 ms'), ('Decay', '0-99'), ('Liveliness', '0-99')],
  'A bigger plate with a longer decay and a smooth, colored tail.', 'Uma placa maior, com decay mais longo e cauda suave e colorida.'),
 ('Room', 'Stereo', [('Pre Delay', '0-100 ms'), ('Decay', '0-99'), ('Liveliness', '0-99')],
  'A small room with strong early reflections. Good for dialog, voice-over and for adding body to electric guitar.', 'Uma sala pequena com reflexões iniciais fortes. Boa para locução e diálogos e para dar corpo à guitarra elétrica.'),
 ('Chamber', 'Stereo', [('Pre Delay', '0-100 ms'), ('Decay', '0-99'), ('Liveliness', '0-99')],
  'An even reverb with little colour as it decays. Works on almost anything, especially the spoken voice.', 'Um reverb uniforme, com pouca coloração enquanto decai. Funciona em quase tudo, principalmente na voz falada.'),
 ('Gated', 'Mono in / stereo out', [('Pre Delay', '0-200 ms'), ('Decay (hold time)', '0-60'), ('Diffusion', '0-99')],
  'A dense burst of reverb that is cut off abruptly. The Decay knob sets how long the gate stays open. Great on snare and toms.', 'Uma explosão densa de reverb que é cortada de repente. O knob Decay define por quanto tempo o gate fica aberto. Ótimo em caixa e tons.'),
 ('Reverse', 'Mono in / stereo out', [('Pre Delay', '0-200 ms'), ('Decay (segment length)', '0-60'), ('Diffusion', '0-99')],
  'The tail swells up and then stops, the opposite of a normal reverb. Use for special effects and transitions.', 'A cauda cresce e depois para, o oposto de um reverb normal. Use em efeitos especiais e transições.'),
 ('Vocal Hall', 'Stereo', [('Pre Delay', '0-200 ms'), ('Decay', '0-99'), ('Liveliness', '0-99')],
  'A hall tuned for voices: lower diffusion, so soft transients stay clear.', 'Um salão ajustado para vozes: menor difusão, para manter os transientes suaves claros.'),
 ('Vocal Plate', 'Stereo', [('Pre Delay', '0-200 ms'), ('Decay', '0-99'), ('Liveliness', '0-99')],
  'A plate tuned for voices: smooth and present without hiding the words.', 'Uma placa ajustada para vozes: suave e presente, sem esconder as palavras.'),
 ('Drum Hall', 'Stereo', [('Pre Delay', '0-100 ms'), ('Decay', '0-99'), ('Liveliness', '0-99')],
  'A hall with higher diffusion, to smooth out fast drum and percussion transients.', 'Um salão com maior difusão, para suavizar os transientes rápidos de bateria e percussão.'),
 ('Drum Plate', 'Stereo', [('Pre Delay', '0-100 ms'), ('Decay', '0-99'), ('Liveliness', '0-99')],
  'A very dense, bright plate for drums.', 'Uma placa muito densa e brilhante para bateria.'),
 ('Ambience', 'Stereo', [('Pre Delay', '0-100 ms'), ('Shape', '0-99'), ('Liveliness', '0-99')],
  'The feeling of a small or medium space without an audible decay. Shape moves the energy from the early reflections to the tail.', 'A sensação de um espaço pequeno ou médio sem decay audível. Shape move a energia das reflexões iniciais para a cauda.'),
 ('Studio', 'Stereo', [('Pre Delay', '0-100 ms'), ('Decay', '0-99'), ('Liveliness', '0-99')],
  'A well-controlled recording room, like the live rooms of a studio. Useful on voice and on single instruments.', 'Uma sala de gravação bem controlada, como as salas ao vivo de um estúdio. Útil em voz e em instrumentos individuais.'),
 ('Arena', 'Stereo', [('Pre Delay', '0-200 ms'), ('Decay', '0-99'), ('Liveliness', '0-99')],
  'A huge space with very long reflections and little high-frequency content. A special-effect reverb: keep it low in a busy mix.', 'Um espaço enorme com reflexões muito longas e pouco conteúdo de agudos. Um reverb de efeito especial: use baixo em mixagens cheias.'),
 ('Spring', 'Mono in / stereo out', [('Pre Delay', '0-200 ms'), ('Decay', '0-99'), ('Boing', '0-99')],
  'A spring tank. Boing sets how much the springs rattle when hit by loud transients: the classic rockabilly and guitar-amp sound.', 'Um tanque de molas. Boing define o quanto as molas "chacoalham" com transientes fortes: o som clássico de amplificador de guitarra e rockabilly.'),
]
DELAYS = [
 ('Studio Delay', 'Stereo', [('Time Range', '0-72'), ('Feedback', '0-99 + Hold'), ('Duck Threshold', '-70 to 0 dB')],
  'Stereo delay up to 2.5 s with a ducker: while the input is loud, the repeats are lowered by 6 dB so they do not muddy the original. Feedback fully up holds the repeats.', 'Delay estéreo de até 2,5 s com ducker: enquanto a entrada está alta, as repetições são reduzidas em 6 dB para não embolar o original. Feedback no máximo congela as repetições.'),
 ('Digital Delay', 'Mono in / stereo out', [('Time Range', '0-72'), ('Feedback', '0-99 + Hold'), ('Duck Threshold', '-70 to 0 dB')],
  'The cleanest delay: mono delay up to 5 s, exact repeats, with the ducker.', 'O delay mais limpo: delay mono de até 5 s, repetições exatas, com o ducker.'),
 ('Tape Delay', 'Mono in / stereo out', [('Time Range', '0-72'), ('Feedback', '0-99'), ('Smear', '0-99')],
  'Warm delay up to 5 s with wow and flutter, saturation and high-frequency loss in every repeat. Smear darkens and blurs the repeats.', 'Delay quente de até 5 s com wow e flutter, saturação e perda de agudos a cada repetição. Smear escurece e "borra" as repetições.'),
 ('Pong Delay', 'Mono in / stereo out', [('Time Range', '0-72'), ('Feedback', '0-99 + Hold'), ('Tap Ratio', '0-23')],
  'A ping-pong delay: the repeats move from left to right. Tap Ratio at 12 gives an even alternation; lower values bring the first tap earlier.', 'Um delay ping-pong: as repetições vão da esquerda para a direita. Tap Ratio em 12 dá uma alternância uniforme; valores menores antecipam o primeiro tap.'),
 ('Mod Delay', 'Stereo', [('Time Range', '0-72'), ('Feedback', '0-99'), ('Mod Depth', '0-99')],
  'A stereo delay with a chorus-like movement in the repeats. Up to 2.5 s.', 'Um delay estéreo com movimento tipo chorus nas repetições. Até 2,5 s.'),
 ('Reverse Delay', 'Mono in / stereo out', [('Time Range', '0-72'), ('Feedback', '0-99'), ('Smear', '0-99')],
  'Backwards repeats that build up from soft to loud, as if the delay came before the sound. Up to 5 s.', 'Repetições ao contrário que crescem do suave para o forte, como se o delay viesse antes do som. Até 5 s.'),
]
DYNAMICS = [
 ('De-Esser', 'Mono wet only', [('Ratio', '1:1 to Inf:1'), ('Freq Range', '1600 Hz to 10 kHz (41 steps)'), ('Bandwidth', '1/4 to 1/24 octave (0-5)')],
  'Reduces harsh "s" sounds. Only the chosen frequency band is compressed, so the rest of the voice is untouched. Set Freq Range on the sibilance, then raise Ratio.', 'Reduz os sons "s" ásperos. Só a banda de frequência escolhida é comprimida, então o resto da voz fica intacto. Ajuste Freq Range na sibilância e depois aumente o Ratio.'),
 ('Compressor', 'Mono wet only', [('Threshold', '-70 to 0 dB'), ('Ratio', '1:1 to Inf:1'), ('Gain', '0 to +12 dB')],
  'A smooth compressor with a soft knee. Gain restores the level lost to compression. Because it is a wet-only effect, the Mix knob works as a level control.', 'Um compressor suave com soft knee. Gain devolve o nível perdido na compressão. Como é um efeito só wet, o knob Mix funciona como controle de nível.'),
]
MODS = [
 ('Chorus', 'Stereo', [('Speed', '0-99'), ('Depth', '0-99'), ('Voices', '0-99')],
  'Thickens the sound by mixing delayed copies that move slowly in pitch. Voices adds up to 8 extra voices.', 'Engrossa o som misturando cópias atrasadas que variam devagar de afinação. Voices adiciona até 8 vozes extras.'),
 ('Flanger', 'Stereo', [('Speed', '0-99'), ('Depth', '0-99'), ('Regen', '0-99')],
  'A sweeping comb filter, from a light swish to a jet-plane sound. Regen adds a metallic resonance.', 'Um filtro de pente em varredura, de um leve "swish" a um som de jato. Regen adiciona ressonância metálica.'),
 ('Phaser', 'Stereo', [('Speed', '0-99'), ('Depth', '0-99'), ('Regen', '0-99')],
  'Notches that move up and down the spectrum. Great on keyboards, pads and guitar.', 'Entalhes que sobem e descem pelo espectro. Ótimo em teclados, pads e guitarra.'),
 ('Tremolo/Pan', 'Stereo wet only', [('Speed', '0-99'), ('Depth', '0-99'), ('Phase', '0-99')],
  'Rhythmic level changes. With Phase at 0 both channels move together (tremolo); at 99 they alternate (auto-pan).', 'Variações rítmicas de nível. Com Phase em 0 os dois canais se movem juntos (tremolo); em 99 eles alternam (auto-pan).'),
 ('Rotary', 'Mono in / stereo out, wet only', [('Speed', '0-99'), ('Doppler', '0-99'), ('Spread', '0-99')],
  'A rotating-speaker cabinet with a horn and a drum rotor that speed up and slow down with inertia. Doppler sets the pitch effect, Spread the stereo image.', 'Um gabinete de alto-falante rotativo com rotor de agudos (horn) e de graves (drum) que aceleram e desaceleram com inércia. Doppler define o efeito de afinação, Spread a imagem estéreo.'),
 ('Vibrato', 'Stereo wet only', [('Speed', '0-99'), ('Depth', '0-99'), ('Phase', '0-99')],
  'A smooth pitch variation. Phase offsets left and right for a panning motion.', 'Uma variação suave de afinação. Phase defasa esquerda e direita para um movimento de panorama.'),
 ('Pitch Shift', 'Stereo', [('Shift', '0-24 semitones'), ('Up / Down', 'Up or Down'), ('Cutoff Freq', '0-55')],
  'Shifts the pitch by up to two octaves, up or down. Cutoff Freq rolls off the highs of the shifted signal.', 'Muda a afinação em até duas oitavas, para cima ou para baixo. Cutoff Freq corta os agudos do sinal alterado.'),
 ('Detune', 'Stereo', [('Left Shift', '-24 to +24 cents'), ('Right Shift', '-24 to +24 cents'), ('Spread', '0-99')],
  'A slightly detuned copy for a "double-tracked" sound. Spread moves the two detuned voices apart in the stereo field.', 'Uma cópia levemente desafinada para um som de "dobra". Spread afasta as duas vozes desafinadas no campo estéreo.'),
]

# ------------------------------------------------------------------ page decoration
class Doc(BaseDocTemplate):
    def __init__(self, path, lang, **kw):
        super().__init__(path, pagesize=A4, leftMargin=M, rightMargin=M, topMargin=24 * mm, bottomMargin=18 * mm,
                         title='NF Processor Effects - ' + ('User Manual' if lang == 'en' else 'Manual do Usuário'), author='NF Audio Tools by Nenno Fernando', **kw)
        self.lang = lang
        fr = Frame(M, 18 * mm, W - 2 * M, H - 42 * mm, id='f', leftPadding=0, rightPadding=0, topPadding=0, bottomPadding=0)
        self.addPageTemplates([PageTemplate(id='cover', frames=[Frame(M, 18 * mm, W - 2 * M, H - 36 * mm, leftPadding=0, rightPadding=0, topPadding=0, bottomPadding=0)], onPage=self.cover),
                               PageTemplate(id='body', frames=[fr], onPage=self.body)])
    def cover(self, c, d):
        c.setFillColor(BLUE); c.rect(0, 0, W, H, fill=1, stroke=0)
        c.setFillColor(DARK); c.rect(0, 0, W, 16 * mm, fill=1, stroke=0)
        c.setFillColor(white); c.setFont('Helvetica', 9)
        c.drawCentredString(W / 2, 6 * mm, 'NF Audio Tools by Nenno Fernando  -  (C) %s NF Audio Tools. All rights reserved.' % '2026')
    def body(self, c, d):
        c.setFillColor(BLUE); c.rect(0, H - 14 * mm, W, 14 * mm, fill=1, stroke=0)
        c.setFillColor(white); c.setFont('Helvetica-Bold', 10); c.drawString(M, H - 9 * mm, 'NF PROCESSOR EFFECTS')
        c.setFont('Helvetica', 9); c.drawRightString(W - M, H - 9 * mm, ('User Manual' if self.lang == 'en' else 'Manual do Usuário') + '  -  V' + VERSION)
        c.setFillColor(GREY); c.setFont('Helvetica', 8.5)
        c.drawString(M, 9 * mm, 'NF Audio Tools by Nenno Fernando')
        c.drawRightString(W - M, 9 * mm, ('Page %d' if self.lang == 'en' else 'Página %d') % (d.page))
    def afterFlowable(self, f):
        if isinstance(f, Paragraph) and f.style.name in ('H1', 'H2'):
            lvl = 0 if f.style.name == 'H1' else 1
            self.notify('TOCEntry', (lvl, f.getPlainText(), self.page))

ST = {
 'H1': ParagraphStyle('H1', fontName='Helvetica-Bold', fontSize=20, leading=24, textColor=BLUE, spaceBefore=16, spaceAfter=8),
 'H2': ParagraphStyle('H2', fontName='Helvetica-Bold', fontSize=13, leading=16, textColor=CYAN, spaceBefore=10, spaceAfter=4),
 'B': ParagraphStyle('B', fontName='Helvetica', fontSize=10, leading=14.2, textColor=INK, spaceAfter=5),
 'BL': ParagraphStyle('BL', fontName='Helvetica', fontSize=10, leading=14, textColor=INK, leftIndent=12, bulletIndent=2, spaceAfter=2.5),
 'S': ParagraphStyle('S', fontName='Helvetica', fontSize=8.6, leading=11, textColor=INK),
 'SB': ParagraphStyle('SB', fontName='Helvetica-Bold', fontSize=8.6, leading=11, textColor=INK),
 'TH': ParagraphStyle('TH', fontName='Helvetica-Bold', fontSize=8.6, leading=11, textColor=white),
 'CAP': ParagraphStyle('CAP', fontName='Helvetica-Oblique', fontSize=8.5, leading=11, textColor=GREY, alignment=TA_CENTER, spaceAfter=8),
 'NOTE': ParagraphStyle('NOTE', fontName='Helvetica', fontSize=9.4, leading=13, textColor=DARK),
 'TOC1': ParagraphStyle('TOC1', fontName='Helvetica-Bold', fontSize=10.5, leading=17, textColor=INK),
 'TOC2': ParagraphStyle('TOC2', fontName='Helvetica', fontSize=9.5, leading=13.5, textColor=GREY, leftIndent=14),
}
def esc(s): return s.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')

def img(name, width_mm, caption=None):
    p = os.path.join(IMG, name); w, h = Image.open(p).size
    out = [RLImage(p, width=width_mm * mm, height=width_mm * mm * h / w)]
    if caption: out.append(Paragraph(caption, ST['CAP']))
    return out

def table(rows, widths_mm, header=True, small=True):
    data = [[Paragraph(esc(str(c)), ST['TH'] if (header and i == 0) else ST['S']) for c in r] for i, r in enumerate(rows)]
    t = Table(data, colWidths=[w * mm for w in widths_mm], repeatRows=1 if header else 0)
    style = [('VALIGN', (0, 0), (-1, -1), 'MIDDLE'), ('GRID', (0, 0), (-1, -1), 0.4, HexColor('#b7c6e0')),
             ('TOPPADDING', (0, 0), (-1, -1), 3), ('BOTTOMPADDING', (0, 0), (-1, -1), 3), ('LEFTPADDING', (0, 0), (-1, -1), 4), ('RIGHTPADDING', (0, 0), (-1, -1), 4)]
    if header: style += [('BACKGROUND', (0, 0), (-1, 0), BLUE)]
    for i in range(1 if header else 0, len(rows)):
        if i % 2 == 0: style.append(('BACKGROUND', (0, i), (-1, i), LIGHT))
    t.setStyle(TableStyle(style))
    return t

def note(text):
    t = Table([[Paragraph(text, ST['NOTE'])]], colWidths=[W - 2 * M])
    t.setStyle(TableStyle([('BACKGROUND', (0, 0), (-1, -1), LIGHT), ('LINEBEFORE', (0, 0), (0, -1), 3, ORANGE), ('LEFTPADDING', (0, 0), (-1, -1), 9),
                           ('TOPPADDING', (0, 0), (-1, -1), 6), ('BOTTOMPADDING', (0, 0), (-1, -1), 6)]))
    return t

class Routings(Flowable):
    """The four routings as small block diagrams."""
    def __init__(self, labels): super().__init__(); self.labels = labels; self.width = W - 2 * M; self.height = 62 * mm
    def draw(self):
        c = self.canv; cw = self.width / 2
        def box(x, y, t):
            c.setFillColor(BLUE); c.roundRect(x, y, 26 * mm, 8 * mm, 2, fill=1, stroke=0)
            c.setFillColor(white); c.setFont('Helvetica-Bold', 8); c.drawCentredString(x + 13 * mm, y + 2.6 * mm, t)
        def line(x1, y1, x2, y2):
            c.setStrokeColor(INK); c.setLineWidth(1); c.line(x1, y1, x2, y2)
        def tag(x, y, t): c.setFillColor(INK); c.setFont('Helvetica', 7.5); c.drawString(x, y, t)
        for k in range(4):
            ox = (k % 2) * cw; oy = self.height - (k // 2 + 1) * 30 * mm
            c.setFillColor(INK); c.setFont('Helvetica-Bold', 9.5); c.drawString(ox, oy + 24 * mm, self.labels[k])
            if k == 0:
                box(ox + 18 * mm, oy + 13 * mm, 'Processor 1'); box(ox + 18 * mm, oy + 2 * mm, 'Processor 2')
                tag(ox + 2 * mm, oy + 16 * mm, 'L in'); tag(ox + 2 * mm, oy + 5 * mm, 'R in'); tag(ox + 47 * mm, oy + 16 * mm, 'L out'); tag(ox + 47 * mm, oy + 5 * mm, 'R out')
            elif k == 1:
                box(ox + 6 * mm, oy + 8 * mm, 'Processor 1'); box(ox + 38 * mm, oy + 8 * mm, 'Processor 2'); line(ox + 32 * mm, oy + 12 * mm, ox + 38 * mm, oy + 12 * mm)
                tag(ox, oy + 11 * mm, 'in'); tag(ox + 66 * mm, oy + 11 * mm, 'out')
            elif k == 2:
                box(ox + 18 * mm, oy + 13 * mm, 'Processor 1'); box(ox + 18 * mm, oy + 2 * mm, 'Processor 2'); line(ox + 46 * mm, oy + 17 * mm, ox + 52 * mm, oy + 17 * mm)
                line(ox + 46 * mm, oy + 6 * mm, ox + 52 * mm, oy + 6 * mm); line(ox + 52 * mm, oy + 6 * mm, ox + 52 * mm, oy + 17 * mm); tag(ox + 54 * mm, oy + 10 * mm, 'L + R')
                tag(ox, oy + 11 * mm, 'L + R in')
            else:
                box(ox + 18 * mm, oy + 13 * mm, 'Processor 1'); box(ox + 18 * mm, oy + 2 * mm, 'Processor 2'); line(ox + 46 * mm, oy + 17 * mm, ox + 52 * mm, oy + 17 * mm)
                line(ox + 46 * mm, oy + 6 * mm, ox + 52 * mm, oy + 6 * mm); line(ox + 52 * mm, oy + 6 * mm, ox + 52 * mm, oy + 17 * mm); tag(ox + 54 * mm, oy + 10 * mm, 'L + R')
                tag(ox + 2 * mm, oy + 16 * mm, 'L in'); tag(ox + 2 * mm, oy + 5 * mm, 'R in')

# ------------------------------------------------------------------ content
def build(lang):
    L = (lambda en, pt: en) if lang == 'en' else (lambda en, pt: pt)
    P = lambda t, st='B': Paragraph(t, ST[st])
    bullets = lambda items: [Paragraph(t, ST['BL'], bulletText='•') for t in items]
    s = []
    # ---- cover
    s.append(Spacer(1, 6 * mm))
    s += img('../../../Assets/logo_nf_audio_tools_official.png', 62)
    s.append(Spacer(1, 6 * mm))
    s.append(Paragraph('NF PROCESSOR EFFECTS', ParagraphStyle('t', fontName='Helvetica-Bold', fontSize=32, leading=36, textColor=white)))
    s.append(Paragraph(L('Dual reverb / effects processor', 'Processador duplo de reverb / efeitos'), ParagraphStyle('t2', fontName='Helvetica', fontSize=15, leading=20, textColor=HexColor('#cfe0ff'), spaceBefore=4)))
    s.append(Paragraph(L('User Manual', 'Manual do Usuário') + '  -  V' + VERSION, ParagraphStyle('t3', fontName='Helvetica-Bold', fontSize=12, leading=16, textColor=white, spaceBefore=10)))
    s.append(Spacer(1, 12 * mm))
    s += img('full.png', 174)
    s.append(Spacer(1, 6 * mm))
    s.append(Paragraph(L('VST3  -  Audio Unit  -  AAX (Pro Tools)', 'VST3  -  Audio Unit  -  AAX (Pro Tools)'), ParagraphStyle('t4', fontName='Helvetica', fontSize=11, textColor=white)))
    s.append(NextPageTemplate('body')); s.append(PageBreak())

    # ---- contents
    s.append(Paragraph(L('Contents', 'Sumário'), ParagraphStyle('CT', parent=ST['H1'])))
    toc = TableOfContents(); toc.levelStyles = [ST['TOC1'], ST['TOC2']]; toc.dotsMinLevel = 0
    s.append(toc); s.append(PageBreak())

    # ---- 1 introduction
    s.append(P(L('1. Introduction', '1. Introdução'), 'H1'))
    s.append(P(L('NF Processor Effects is a dual reverb and effects processor. It has two independent processors, and each one can hold any of 32 effects: 16 reverbs, 6 delays, 8 modulation and pitch effects and 2 dynamics processors. The two processors can be combined in four different ways (routings), so you can use them as two separate effects, one after the other, or side by side.',
                 'O NF Processor Effects é um processador duplo de reverb e efeitos. Ele tem dois processadores independentes, e cada um pode receber qualquer um de 32 efeitos: 16 reverbs, 6 delays, 8 efeitos de modulação e afinação e 2 processadores de dinâmica. Os dois processadores podem ser combinados de quatro maneiras (roteamentos): como dois efeitos separados, um depois do outro, ou lado a lado.')))
    s += bullets([L('<b>32 effects</b>: reverbs (halls, plates, room, chamber, gated, reverse, ambience, studio, arena, spring), delays (studio, digital, tape, pong, modulated, reverse), de-esser, compressor, chorus, flanger, phaser, tremolo/pan, rotary, vibrato, pitch shift and detune.',
                    '<b>32 efeitos</b>: reverbs (halls, plates, room, chamber, gated, reverse, ambience, studio, arena, spring), delays (studio, digital, tape, pong, modulated, reverse), de-esser, compressor, chorus, flanger, phaser, tremolo/pan, rotary, vibrato, pitch shift e detune.'),
                  L('<b>4 routings</b>: Dual Mono, Cascade, Dual Stereo (parallel) and Mono Split.', '<b>4 roteamentos</b>: Dual Mono, Cascade, Dual Stereo (paralelo) e Mono Split.'),
                  L('<b>99 factory programs</b> and <b>99 user programs</b>, with Store.', '<b>99 programas de fábrica</b> e <b>99 programas de usuário</b>, com Store.'),
                  L('Three controls per processor that change meaning with the effect, plus Mix, Bypass and Tap Tempo.', 'Três controles por processador que mudam de função conforme o efeito, mais Mix, Bypass e Tap Tempo.'),
                  L('Audition: five built-in sounds to try a setting without any audio source.', 'Audition: cinco sons internos para testar um ajuste sem nenhuma fonte de áudio.'),
                  L('Formats: VST3, Audio Unit and AAX, for macOS 10.15 or later (Apple Silicon and Intel).', 'Formatos: VST3, Audio Unit e AAX, para macOS 10.15 ou superior (Apple Silicon e Intel).')])
    s.append(Spacer(1, 3 * mm))
    s.append(note(L('<b>About the sound.</b> The algorithms of NF Processor Effects were designed by NF Audio Tools to follow the description of each effect in this manual. The factory programs are starting points: use the knobs to adjust each one to your material.',
                    '<b>Sobre o som.</b> Os algoritmos do NF Processor Effects foram criados pela NF Audio Tools seguindo a descrição de cada efeito neste manual. Os programas de fábrica são pontos de partida: use os knobs para ajustar cada um ao seu material.')))

    # ---- 2 installation
    s.append(P(L('2. Installation', '2. Instalação'), 'H1'))
    s.append(P(L('Open the DMG and double-click <b>Install NF Processor Effects</b>. The installer lets you choose which formats to install on the <b>Customize</b> step. The installer is not signed by Apple: the first time, right-click the installer and choose <b>Open</b> (or use <b>System Settings > Privacy &amp; Security > Open Anyway</b>).',
                 'Abra o DMG e dê dois cliques em <b>Install NF Processor Effects</b>. O instalador permite escolher os formatos na etapa <b>Customize</b>. O instalador não é assinado pela Apple: na primeira vez, clique com o botão direito no instalador e escolha <b>Abrir</b> (ou use <b>Ajustes do Sistema > Privacidade e Segurança > Abrir Mesmo Assim</b>).')))
    s.append(table([[L('Format', 'Formato'), L('Location', 'Local')],
                    ['VST3', '/Library/Audio/Plug-Ins/VST3/NF Processor Effects.vst3'],
                    ['Audio Unit', '/Library/Audio/Plug-Ins/Components/NF Processor Effects.component'],
                    ['AAX', '/Library/Application Support/Avid/Audio/Plug-Ins/NF Processor Effects.aaxplugin']], [30, 144]))
    s.append(Spacer(1, 3 * mm))
    s.append(P(L('Restart your DAW and rescan the plug-ins if it does not appear. The plug-in is listed under <b>Reverb</b> (manufacturer <b>NF Audio Tools</b>). Logic Pro and Luna use the Audio Unit; Pro Tools uses AAX.',
                 'Reinicie a DAW e refaça o scan de plugins se ele não aparecer. O plugin fica na categoria <b>Reverb</b> (fabricante <b>NF Audio Tools</b>). O Logic Pro e a Luna usam o Audio Unit; o Pro Tools usa AAX.')))

    # ---- 3 quick start
    s.append(P(L('3. Quick start', '3. Início rápido'), 'H1'))
    s.append(P(L('Reverbs and delays are normally used as <b>send effects</b> (parallel); modulation effects and dynamics are normally used as <b>inserts</b> (in series). The plug-in works both ways.',
                 'Reverbs e delays normalmente são usados como <b>efeitos de send</b> (paralelo); efeitos de modulação e dinâmica normalmente são usados como <b>inserts</b> (em série). O plugin funciona das duas formas.')))
    s.append(P(L('As an insert (guitar, vocal, instrument track)', 'Como insert (trilha de guitarra, voz, instrumento)'), 'H2'))
    s += bullets([L('Put the plug-in in an insert slot of the track.', 'Coloque o plugin em um slot de insert da trilha.'),
                  L('Choose a program with the <b>Program Select</b> knob, or with the preset tab at the top right (the default factory bank is <b>Serial</b>, made for this use).', 'Escolha um programa com o knob <b>Program Select</b> ou pela aba de preset no topo à direita (o banco de fábrica padrão é <b>Serial</b>, feito para este uso).'),
                  L('Adjust <b>Input</b> so the input LEDs light only occasionally in red, and <b>Mix 1</b> / <b>Mix 2</b> for the amount of effect.', 'Ajuste o <b>Input</b> para que os LEDs de entrada só acendam em vermelho ocasionalmente, e <b>Mix 1</b> / <b>Mix 2</b> para a quantidade de efeito.')])
    s.append(P(L('As a send effect (bus / aux)', 'Como send (bus / aux)'), 'H2'))
    s += bullets([L('Insert the plug-in on an aux or FX track and send audio to it from the other tracks.', 'Insira o plugin em uma trilha aux ou FX e envie áudio para ela a partir das outras trilhas.'),
                  L('In the menu (three lines) choose <b>Factory programs: Parallel bank</b>. In this bank the Mix knobs are set to 100 % wet, which is what a send needs.', 'No menu (três linhas) escolha <b>Factory programs: Parallel bank</b>. Neste banco os knobs Mix ficam em 100 % wet, que é o que um send precisa.'),
                  L('Use the routing <b>3 (Dual Stereo)</b> or <b>4 (Mono Split)</b> for two effects side by side.', 'Use o roteamento <b>3 (Dual Stereo)</b> ou <b>4 (Mono Split)</b> para dois efeitos lado a lado.')])
    s.append(P(L('Try a setting without a source', 'Experimente um ajuste sem fonte de áudio'), 'H2'))
    s.append(P(L('Press <b>Audition</b>: a short built-in sound (drum stick, snare, kick, voice, guitar) plays through the processors.', 'Pressione <b>Audition</b>: um som interno curto (baqueta, caixa, bumbo, voz, guitarra) toca através dos processadores.')))

    # ---- 4 front panel
    s.append(P(L('4. The front panel', '4. O painel frontal'), 'H1'))
    s += img('full_numbered.png', 174, L('Figure 1 - The panel with numbered controls.', 'Figura 1 - O painel com os controles numerados.'))
    rows = [[L('No.', 'Nº'), L('Control', 'Controle'), L('What it does', 'O que faz')],
      ['1', L('Input meters', 'Medidores de entrada'), L('Two columns of six LEDs (left and right input level). Red = the signal is at its limit.', 'Duas colunas de seis LEDs (nível de entrada esquerdo e direito). Vermelho = o sinal está no limite.')],
      ['2', 'Input', L('Input level of both channels. Twelve o\'clock = 0 dB.', 'Nível de entrada dos dois canais. Meio-dia = 0 dB.')],
      ['3', 'Mix 1', L('Wet/dry mix of Processor 1 (level control for wet-only effects).', 'Mix wet/dry do Processor 1 (controle de nível nos efeitos só wet).')],
      ['4', 'Mix 2', L('Wet/dry mix of Processor 2.', 'Mix wet/dry do Processor 2.')],
      ['5', 'Routing', L('Cycles through the four routings. The small diagram shows the current one.', 'Alterna entre os quatro roteamentos. O pequeno diagrama mostra o atual.')],
      ['6', L('Reverb matrix', 'Matriz de reverbs'), L('Shows the active reverbs: green LED = Processor 1, red LED = Processor 2.', 'Mostra os reverbs ativos: LED verde = Processor 1, LED vermelho = Processor 2.')],
      ['7', L('Delay / effects matrix', 'Matriz de delays / efeitos'), L('The same for delays, dynamics and modulation effects.', 'O mesmo para delays, dinâmica e efeitos de modulação.')],
      ['8', L('Preset tab', 'Aba de preset'), L('Name of the program. Arrows step through the factory programs; click the name for the list, Save, Load and Restore default.', 'Nome do programa. As setas passam pelos programas de fábrica; clique no nome para a lista, Save, Load e Restore default.')],
      ['9', L('Menu (three lines)', 'Menu (três linhas)'), L('Utility settings, audition sound and About.', 'Ajustes utilitários, som do audition e About.')],
      ['10', 'Store', L('Stores the current settings in one of the 99 user programs.', 'Guarda os ajustes atuais em um dos 99 programas de usuário.')],
      ['11', 'Audition', L('Plays a built-in sound through the processors. Shift-click chooses the sound.', 'Toca um som interno através dos processadores. Shift+clique escolhe o som.')],
      ['12', L('Program display', 'Display de programa'), L('Two-digit display: program number (dot lit = user program) and the value of the knob you are turning.', 'Display de dois dígitos: número do programa (ponto aceso = programa de usuário) e o valor do knob que você está girando.')],
      ['13', 'Program Select', L('Drag or use the wheel to choose a program (the number blinks); click it to load it.', 'Arraste ou use a roda do mouse para escolher um programa (o número pisca); clique para carregá-lo.')],
      ['14', 'Effect Select', L('The up/down buttons choose the effect of the processor.', 'Os botões para cima/baixo escolhem o efeito do processador.')],
      ['15', L('Effect name', 'Nome do efeito'), L('Shows the effect loaded in the processor (green = Processor 1, red = Processor 2).', 'Mostra o efeito carregado no processador (verde = Processor 1, vermelho = Processor 2).')],
      ['16', 'Tempo', L('Tap twice to set the delay tempo. The LED blinks with the tempo.', 'Toque duas vezes para definir o tempo do delay. O LED pisca no tempo.')],
      ['17', 'Bypass', L('Bypasses the processor (lit red = bypassed).', 'Faz o bypass do processador (vermelho aceso = em bypass).')],
      ['18', L('Value readouts', 'Leitura de valores'), L('The value of each knob, in the unit of the effect.', 'O valor de cada knob, na unidade do efeito.')],
      ['19', L('Effect knobs', 'Knobs do efeito'), L('Three knobs whose names and ranges change with the effect (see chapter 7).', 'Três knobs cujos nomes e faixas mudam com o efeito (veja o capítulo 7).')],
      ['20', L('Change LED', 'LED de alteração'), L('Lights when the knob differs from the stored program.', 'Acende quando o knob é diferente do programa guardado.')]]
    s.append(table(rows, [10, 38, 126]))
    s.append(Spacer(1, 3 * mm))
    s.append(P(L('The NF logo (top left) returns the window to its default size after you have resized it with the handle at the bottom right corner.', 'O logo NF (canto superior esquerdo) devolve a janela ao tamanho padrão depois de você redimensioná-la pela alça no canto inferior direito.')))

    # ---- 5 routing
    s.append(PageBreak())
    s.append(P(L('5. Routings and signal flow', '5. Roteamentos e fluxo de sinal'), 'H1'))
    s.append(P(L('Press <b>Routing</b> to cycle through the four routings. The number of the routing is also shown on the program display when you press the button.', 'Pressione <b>Routing</b> para alternar entre os quatro roteamentos. O número do roteamento também aparece no display de programa quando você pressiona o botão.')))
    s.append(Routings(['1  Dual Mono', '2  Cascade', '3  Dual Stereo (parallel)', '4  Mono Split']))
    s.append(table([[L('Routing', 'Roteamento'), L('Description', 'Descrição')],
        ['1  Dual Mono', L('The left input goes through Processor 1 to the left output; the right input goes through Processor 2 to the right output. Two completely independent mono effects.', 'A entrada esquerda passa pelo Processor 1 e vai para a saída esquerda; a entrada direita passa pelo Processor 2 e vai para a saída direita. Dois efeitos mono totalmente independentes.')],
        ['2  Cascade', L('Left and right go through Processor 1 and then through Processor 2, in stereo. The usual choice for inserts.', 'Esquerda e direita passam pelo Processor 1 e depois pelo Processor 2, em estéreo. A escolha usual para inserts.')],
        ['3  Dual Stereo', L('Left and right go through both processors at the same time; the two outputs are mixed together. The usual choice for send / return use.', 'Esquerda e direita passam pelos dois processadores ao mesmo tempo; as duas saídas são misturadas. A escolha usual para uso em send / return.')],
        ['4  Mono Split', L('The left input goes through Processor 1, the right input through Processor 2, and the two outputs are mixed together to both channels.', 'A entrada esquerda passa pelo Processor 1, a direita pelo Processor 2, e as duas saídas são misturadas nos dois canais.')]], [34, 140]))
    s.append(P(L('Wet, dry and wet-only effects', 'Wet, dry e efeitos só wet'), 'H2'))
    s.append(P(L('For most effects the <b>Mix</b> knob blends the dry input with the effect (fully clockwise = 100 % wet). The <b>wet-only</b> effects (De-Esser, Compressor, Tremolo/Pan, Rotary and Vibrato) process the whole signal, so there is nothing to blend: for them Mix works as a <b>level control</b>, and fully clockwise is the normal setting.',
                 'Na maioria dos efeitos o knob <b>Mix</b> mistura a entrada seca com o efeito (todo à direita = 100 % wet). Os efeitos <b>só wet</b> (De-Esser, Compressor, Tremolo/Pan, Rotary e Vibrato) processam o sinal inteiro, então não há o que misturar: para eles o Mix funciona como <b>controle de nível</b>, e todo à direita é o ajuste normal.')))
    s.append(P(L('Some effects take a mono signal (they sum left and right) and give a stereo result: Gated, Reverse, Spring, Digital, Tape, Pong and Reverse delays and Rotary. This is shown as "mono in / stereo out" in chapter 7.', 'Alguns efeitos recebem um sinal mono (somam esquerda e direita) e entregam um resultado estéreo: Gated, Reverse, Spring, os delays Digital, Tape, Pong e Reverse e o Rotary. Isso aparece como "mono in / stereo out" no capítulo 7.')))
    s.append(note(L('<b>Bypass.</b> By default a bypassed processor lets the dry signal through. In the menu you can change it to <b>mute</b>. Do not use mute with the Cascade routing: if a processor is muted, the whole chain is silent.', '<b>Bypass.</b> Por padrão um processador em bypass deixa o sinal seco passar. No menu você pode mudar para <b>mute</b>. Não use mute no roteamento Cascade: se um processador estiver mudo, toda a cadeia fica em silêncio.')))

    # ---- 6 operation
    s.append(P(L('6. Using the plug-in', '6. Usando o plugin'), 'H1'))
    s.append(P(L('Loading programs', 'Carregando programas'), 'H2'))
    s += bullets([L('Drag the <b>Program Select</b> knob (or use the mouse wheel over it). The number blinks while it is only selected; <b>click the knob</b> to load it. Programs 1-99 are the factory programs; the next 99 are the user programs (dot lit).', 'Arraste o knob <b>Program Select</b> (ou use a roda do mouse sobre ele). O número pisca enquanto está apenas selecionado; <b>clique no knob</b> para carregar. Os programas 1-99 são os de fábrica; os 99 seguintes são os de usuário (ponto aceso).'),
                  L('With <b>Program AutoLoad</b> on (menu, the default) the program loads as soon as you select it.', 'Com <b>Program AutoLoad</b> ligado (menu, padrão) o programa carrega assim que você o seleciona.'),
                  L('You can also use the preset tab: arrows for the next / previous factory program, or click the name to pick by application (Vocal, Live Sound, Instrumental, Drums &amp; Percussion, Special) or a user program.', 'Você também pode usar a aba de preset: setas para o próximo / anterior programa de fábrica, ou clique no nome para escolher por aplicação (Vocal, Som ao vivo, Instrumental, Bateria e percussão, Especial) ou um programa de usuário.')])
    s.append(P(L('Editing', 'Editando'), 'H2'))
    s += bullets([L('Choose the effects with the <b>Effect Select</b> buttons. Each effect loads its own default knob positions.', 'Escolha os efeitos com os botões <b>Effect Select</b>. Cada efeito carrega as próprias posições padrão dos knobs.'),
                  L('Turn the three knobs: the program display shows the value while you turn. A small <b>change LED</b> next to the knob lights when the value is different from the stored program, and the red LED near <b>Store</b> shows the program has been edited.', 'Gire os três knobs: o display de programa mostra o valor enquanto você gira. Um pequeno <b>LED de alteração</b> ao lado do knob acende quando o valor é diferente do programa guardado, e o LED vermelho perto de <b>Store</b> mostra que o programa foi editado.'),
                  L('Double-click a knob to return it to the default value of the effect. Use the mouse wheel or Shift for fine adjustments.', 'Dê dois cliques em um knob para devolvê-lo ao valor padrão do efeito. Use a roda do mouse ou Shift para ajustes finos.')])
    s.append(P(L('Tap tempo', 'Tap tempo'), 'H2'))
    s.append(P(L('When a delay is loaded, press <b>Tempo</b> twice in time with the music. The LED blinks at the new tempo, and the delay times follow it. <b>Time Range</b> sets the length relative to the tempo: 36 (twelve o\'clock) is one quarter note, 12 is an eighth, 24 a dotted eighth, 54 a dotted quarter and 72 a half note. Each processor has its own tempo. In the menu, <b>Sync delays to the host tempo</b> makes both follow the tempo of the DAW.',
                 'Quando um delay estiver carregado, pressione <b>Tempo</b> duas vezes no ritmo da música. O LED pisca no novo tempo e os tempos do delay o acompanham. <b>Time Range</b> define a duração relativa ao tempo: 36 (meio-dia) é uma semínima, 12 uma colcheia, 24 uma colcheia pontuada, 54 uma semínima pontuada e 72 uma mínima. Cada processador tem o próprio tempo. No menu, <b>Sync delays to the host tempo</b> faz os dois seguirem o tempo da DAW.')))
    s.append(P(L('Storing programs', 'Guardando programas'), 'H2'))
    s += bullets([L('Press <b>Store</b>. The display alternates between "St" and the user program number.', 'Pressione <b>Store</b>. O display alterna entre "St" e o número do programa de usuário.'),
                  L('Drag the <b>Program Select</b> knob to choose the user location (1-99).', 'Arraste o knob <b>Program Select</b> para escolher o local de usuário (1-99).'),
                  L('Press <b>Store</b> again to save. Any other button cancels.', 'Pressione <b>Store</b> de novo para salvar. Qualquer outro botão cancela.')])
    s.append(P(L('User programs are saved as files in <i>Documents/NF Audio Tools/NF Processor Effects/User Programs</i>, so they are shared by every instance of the plug-in. Complete presets (with every setting) can be saved and loaded from the preset tab (<i>Save Preset File</i> / <i>Load Preset File</i>).', 'Os programas de usuário são salvos como arquivos em <i>Documentos/NF Audio Tools/NF Processor Effects/User Programs</i>, então são compartilhados por todas as instâncias do plugin. Presets completos (com todos os ajustes) podem ser salvos e carregados pela aba de preset (<i>Save Preset File</i> / <i>Load Preset File</i>).')))
    s.append(P(L('Menu (three lines)', 'Menu (três linhas)'), 'H2'))
    s.append(table([[L('Option', 'Opção'), L('What it does', 'O que faz')],
        ['Output: Stereo / Mono', L('Mono sums left and right to both outputs; effects such as Pong then lose their left-to-right movement.', 'Mono soma esquerda e direita nas duas saídas; efeitos como o Pong perdem o movimento da esquerda para a direita.')],
        ['Bypass: pass dry / mute', L('What a bypassed processor does (dry signal passes, or silence).', 'O que um processador em bypass faz (o sinal seco passa, ou silêncio).')],
        ['Factory programs: Serial / Parallel bank', L('Serial: Mix settings for in-line use (default). Parallel: Mix at 100 % wet, for send / return. Applies to the next program you load.', 'Serial: ajustes de Mix para uso em linha (padrão). Parallel: Mix em 100 % wet, para send / return. Vale para o próximo programa que você carregar.')],
        [L('Sync delays to the host tempo', 'Sync delays to the host tempo'), L('Both processors use the tempo of the DAW instead of the tap tempo.', 'Os dois processadores usam o tempo da DAW em vez do tap tempo.')],
        ['Program AutoLoad', L('Loads each program as soon as it is selected.', 'Carrega cada programa assim que é selecionado.')],
        [L('Audition sound', 'Som do Audition'), L('All samples in turn, or a single one: drum stick, snare, kick, female vocal "Doo", acoustic guitar arpeggio.', 'Todos os sons em sequência, ou um só: baqueta, caixa, bumbo, voz feminina "Doo", arpejo de violão.')]], [60, 114]))

    # ---- 7 effects
    s.append(P(L('7. Effects reference', '7. Referência dos efeitos'), 'H1'))
    s.append(P(L('The three knobs of each processor change name and range with the effect. The values below are the ones shown on the display and in the value readouts.', 'Os três knobs de cada processador mudam de nome e faixa com o efeito. Os valores abaixo são os mostrados no display e na leitura de valores.')))
    def effects_table(title_en, title_pt, items, intro_en=None, intro_pt=None):
        s.append(P(L(title_en, title_pt), 'H2'))
        if intro_en: s.append(P(L(intro_en, intro_pt)))
        for name, io, knobs, den, dpt in items:
            blk = [Paragraph('<b>%s</b>  <font color="#5a6678">(%s)</font>' % (esc(name), esc(io if lang == 'en' else io.replace('Mono in / stereo out', 'mono in / stereo out').replace('Stereo', 'estéreo').replace('Mono wet only', 'mono, só wet').replace('wet only', 'só wet').replace('mono in / stereo out', 'entrada mono / saída estéreo').replace('Mono in / estéreo out', 'entrada mono / saída estéreo'))), ST['SB']),
                   Paragraph(L(den, dpt), ST['S']),
                   Paragraph('  |  '.join('<b>%s</b>: %s' % (esc(k), esc(r)) for k, r in knobs), ParagraphStyle('k', parent=ST['S'], textColor=CYAN, spaceAfter=6))]
            s.append(KeepTogether(blk))
    effects_table('7.1 Reverbs', '7.1 Reverbs', REVERBS, 'Pre Delay adds a short time between the sound and the start of the reverb (10-25 ms is natural; more can help a busy mix). Decay is the length of the tail and Liveliness the amount of high frequencies in it.', 'Pre Delay adiciona um pequeno tempo entre o som e o início do reverb (10-25 ms é natural; mais pode ajudar numa mixagem cheia). Decay é a duração da cauda e Liveliness a quantidade de agudos nela.')
    effects_table('7.2 Delays', '7.2 Delays', DELAYS, 'Feedback sets the number of repeats. On the Studio, Digital and Pong delays, turning it fully clockwise engages <b>Hold</b>: the repeats loop forever and new sound is no longer fed in.', 'Feedback define o número de repetições. Nos delays Studio, Digital e Pong, girá-lo todo à direita ativa o <b>Hold</b>: as repetições ficam em loop e o som novo deixa de entrar.')
    effects_table('7.3 Dynamics', '7.3 Dinâmica', DYNAMICS)
    effects_table('7.4 Modulation and pitch', '7.4 Modulação e afinação', MODS)

    # ---- 8 programs
    s.append(P(L('8. Factory programs', '8. Programas de fábrica'), 'H1'))
    s.append(P(L('The 99 factory programs combine the two processors in the listed routing. They are starting points: the knob values are general settings for each effect and the Mix values depend on the factory bank (Serial or Parallel). An asterisk (*) means the effect is loaded in bypass.', 'Os 99 programas de fábrica combinam os dois processadores no roteamento indicado. São pontos de partida: os valores dos knobs são ajustes gerais de cada efeito e os valores de Mix dependem do banco de fábrica (Serial ou Parallel). Um asterisco (*) significa que o efeito é carregado em bypass.')))
    rows = [['#', L('Title', 'Título'), L('Application', 'Aplicação'), 'Processor 1', 'Processor 2', L('Rtg', 'Rot.')]]
    for i, (t, app, p1, p2, r, mask) in enumerate(PROGRAMS):
        rows.append([str(i + 1), title_case(t), APPS[lang][app], p1 + ('*' if mask & 1 else ''), p2 + ('*' if mask & 2 else ''), str(r)])
    s.append(table(rows, [9, 44, 33, 33, 33, 11]))

    # ---- 9 tips / specs
    s.append(P(L('9. Tips and technical notes', '9. Dicas e notas técnicas'), 'H1'))
    s += bullets([L('Keep the red input LEDs off most of the time, and use <b>Input</b> to adjust the level that feeds the effects.', 'Mantenha os LEDs vermelhos de entrada apagados na maior parte do tempo, e use o <b>Input</b> para ajustar o nível que alimenta os efeitos.'),
                  L('For reverbs on a busy mix try a short <b>Pre Delay</b> and a lower <b>Liveliness</b>.', 'Para reverbs em uma mixagem cheia, experimente um <b>Pre Delay</b> curto e um <b>Liveliness</b> menor.'),
                  L('Use the <b>Studio</b> or <b>Digital</b> delay ducker so the repeats stay out of the way of the singer.', 'Use o ducker dos delays <b>Studio</b> ou <b>Digital</b> para que as repetições não atrapalhem o cantor.'),
                  L('All parameters can be automated in your DAW (effect choice, knobs, mix, bypass, routing, tempo).', 'Todos os parâmetros podem ser automatizados na DAW (escolha do efeito, knobs, mix, bypass, roteamento, tempo).')])
    s.append(table([[L('Specification', 'Especificação'), L('Value', 'Valor')],
        [L('Formats', 'Formatos'), 'VST3, Audio Unit, AAX'],
        [L('Channels', 'Canais'), L('Mono or stereo (in = out)', 'Mono ou estéreo (entrada = saída)')],
        [L('Sample rates', 'Taxas de amostragem'), L('Any (the effects adapt to the sample rate)', 'Qualquer uma (os efeitos se adaptam à taxa)')],
        [L('Latency', 'Latência'), L('Zero. The Reverse reverb and Reverse delay are by nature delayed by their segment length.', 'Zero. O Reverse reverb e o Reverse delay são, por natureza, atrasados pela duração do segmento.')],
        [L('Delay times', 'Tempos de delay'), L('Up to 2.5 s (Studio, Mod) and 5 s (Digital, Tape, Pong, Reverse)', 'Até 2,5 s (Studio, Mod) e 5 s (Digital, Tape, Pong, Reverse)')],
        [L('Programs', 'Programas'), L('99 factory + 99 user', '99 de fábrica + 99 de usuário')],
        [L('Requirements', 'Requisitos'), L('macOS 10.15 or later, Apple Silicon or Intel', 'macOS 10.15 ou superior, Apple Silicon ou Intel')]], [50, 124]))
    s.append(Spacer(1, 6 * mm))
    s.append(P(L('NF Processor Effects (C) 2026 NF Audio Tools by Nenno Fernando. All rights reserved. All other product names mentioned belong to their respective owners.', 'NF Processor Effects (C) 2026 NF Audio Tools by Nenno Fernando. Todos os direitos reservados. Todos os outros nomes de produtos citados pertencem aos seus respectivos proprietários.'), 'CAP'))
    return s

os.makedirs(OUT, exist_ok=True)
for lang, name in (('en', 'NF_ProcessorEffects_Manual_English.pdf'), ('pt', 'NF_ProcessorEffects_Manual_Portugues.pdf')):
    doc = Doc(os.path.join(OUT, name), lang)
    doc.multiBuild(build(lang))
    print('wrote', name)
