#!/usr/bin/env python3
"""Builds docs/LX-6_Manual.pdf and docs/LX-6_Update_History.pdf (needs reportlab).
Usage: python3 tools/make_docs.py"""
import os
from reportlab.lib.pagesizes import A4
from reportlab.lib.units import mm
from reportlab.lib import colors
from reportlab.lib.styles import ParagraphStyle
from reportlab.platypus import (BaseDocTemplate, PageTemplate, Frame, Paragraph, Spacer, Image,
                                Table, TableStyle, PageBreak, KeepTogether)
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FONTS = os.path.join(ROOT, 'Resources', 'fonts')
DOCS = os.path.join(ROOT, 'docs')
VERSION = '0.1.1'
DATE = '30 October 2026'

pdfmetrics.registerFont(TTFont('SG', os.path.join(FONTS, 'SpaceGrotesk-Medium.ttf')))
pdfmetrics.registerFont(TTFont('SG-B', os.path.join(FONTS, 'SpaceGrotesk-Bold.ttf')))
pdfmetrics.registerFont(TTFont('JB', os.path.join(FONTS, 'JetBrainsMono-Medium.ttf')))

INK = colors.HexColor('#15181b')
PANEL = colors.HexColor('#1d2125')
ACCENT = colors.HexColor('#e0623a')
DIM = colors.HexColor('#6b737b')
LINE = colors.HexColor('#d9dde1')
VOICE = ['#e0623a', '#d9b23a', '#4fbf8a', '#4fa3d9', '#a77fe0', '#e07fb0']

body = ParagraphStyle('body', fontName='SG', fontSize=10, leading=14.5, textColor=INK, spaceAfter=6)
small = ParagraphStyle('small', parent=body, fontSize=8.5, leading=12, textColor=DIM)
h1 = ParagraphStyle('h1', fontName='SG-B', fontSize=20, leading=24, textColor=INK, spaceBefore=4, spaceAfter=10)
h2 = ParagraphStyle('h2', fontName='SG-B', fontSize=13, leading=17, textColor=ACCENT, spaceBefore=12, spaceAfter=6)
h3 = ParagraphStyle('h3', fontName='SG-B', fontSize=10.5, leading=14, textColor=INK, spaceBefore=6, spaceAfter=3)
bullet = ParagraphStyle('bullet', parent=body, leftIndent=12, bulletIndent=2, spaceAfter=3)
cell = ParagraphStyle('cell', parent=body, fontSize=9, leading=12.5, spaceAfter=0)
cellb = ParagraphStyle('cellb', parent=cell, fontName='SG-B')


def page_deco(title):
    def draw(c, doc):
        w, h = A4
        c.saveState()
        c.setFillColor(INK)
        c.rect(0, h - 14 * mm, w, 14 * mm, fill=1, stroke=0)
        c.setFillColor(colors.white)
        c.setFont('SG-B', 13)
        c.drawString(18 * mm, h - 9.3 * mm, 'LX-6')
        for i, v in enumerate(VOICE):
            c.setFillColor(colors.HexColor(v))
            c.circle(37 * mm + i * 4.2 * mm, h - 7.8 * mm, 1.2 * mm, fill=1, stroke=0)
        c.setFillColor(colors.HexColor('#9aa3ad'))
        c.setFont('SG', 8.5)
        c.drawRightString(w - 18 * mm, h - 9 * mm, f'{title}  ·  v{VERSION}')
        c.setFillColor(DIM)
        c.setFont('SG', 8)
        c.drawString(18 * mm, 10 * mm, 'ticcoross.gumroad.com  ·  ticcoross@gmail.com')
        c.drawRightString(w - 18 * mm, 10 * mm, str(doc.page))
        c.restoreState()
    return draw


def make_doc(path, title):
    doc = BaseDocTemplate(path, pagesize=A4, leftMargin=18 * mm, rightMargin=18 * mm,
                          topMargin=24 * mm, bottomMargin=20 * mm, title=f'LX-6 {title}', author='Ticco Ross')
    frame = Frame(doc.leftMargin, doc.bottomMargin, doc.width, doc.height, id='f')
    doc.addPageTemplates([PageTemplate(id='p', frames=[frame], onPage=page_deco(title))])
    return doc


def table(rows, widths, header=True):
    data = [[Paragraph(str(c), cellb if (header and r == 0) else cell) for c in row] for r, row in enumerate(rows)]
    t = Table(data, colWidths=widths, hAlign='LEFT')
    style = [('VALIGN', (0, 0), (-1, -1), 'TOP'),
             ('LINEBELOW', (0, 0), (-1, -1), 0.4, LINE),
             ('TOPPADDING', (0, 0), (-1, -1), 4), ('BOTTOMPADDING', (0, 0), (-1, -1), 4),
             ('LEFTPADDING', (0, 0), (-1, -1), 4)]
    if header:
        style.append(('LINEBELOW', (0, 0), (-1, 0), 1.0, INK))
    t.setStyle(TableStyle(style))
    return t


def bullets(items):
    return [Paragraph(i, bullet, bulletText='•') for i in items]


def screenshot(name, width):
    p = os.path.join(DOCS, name)
    if not os.path.exists(p):
        return Spacer(1, 1)
    img = Image(p)
    ratio = img.imageHeight / img.imageWidth
    img.drawWidth = width
    img.drawHeight = width * ratio
    return img


def manual():
    doc = make_doc(os.path.join(DOCS, 'LX-6_Manual.pdf'), 'User Manual')
    W = doc.width
    s = []
    s += [Paragraph('LX-6 User Manual', h1),
          Paragraph('Six-voice digital drum synthesizer  ·  AU / VST3 / standalone  ·  version ' + VERSION, small),
          Spacer(1, 6), screenshot('lx6-ui.png', W), Spacer(1, 8),
          Paragraph('LX-6 runs the sound engine of the open-source Erica Synths LXR drum machine firmware inside your DAW: '
                    'the same oscillators, FM, noise, transients, envelopes, filters and effects, driven by a 16-step sequencer, '
                    'a sound randomizer and, if you own one, a real LXR-02 over MIDI.', body),
          Paragraph('LX-6 is an independent product by Ticco Ross. It is not made, approved or endorsed by Erica Synths.', small)]

    s += [Paragraph('1. Installing', h2)]
    s += [Paragraph('macOS (Apple Silicon and Intel, macOS 11 or later)', h3)]
    s += bullets(['Open <b>LX-6-' + VERSION + '-macOS.pkg</b>. You can choose Audio Unit, VST3 and the standalone app.',
                  'The installer is not notarized yet. If macOS refuses to open it, right-click the .pkg and choose <b>Open</b>, '
                  'or allow it in System Settings › Privacy &amp; Security.',
                  'Plugins go to /Library/Audio/Plug-Ins/Components (AU) and /Library/Audio/Plug-Ins/VST3. Rescan plugins in your DAW.'])
    s += [Paragraph('Windows (64-bit)', h3)]
    s += bullets(['Run <b>LX-6-' + VERSION + '-Windows.exe</b>. The VST3 goes to C:\\Program Files\\Common Files\\VST3, '
                  'the app to C:\\Program Files\\Ticco Ross\\LX-6.'])
    s += [Paragraph('LX-6 is an instrument: put it on an instrument / MIDI track. In Ableton Live it appears under Plug-Ins › Ticco Ross.', body)]

    s += [Paragraph('2. Quick start', h2)]
    s += bullets(['Press play in your DAW. Pattern A holds a starter beat; the sequencer follows the DAW tempo and position.',
                  'In the standalone app (or with the DAW stopped) press <b>PLAY</b> in the sequencer bar and drag the TEMPO display up/down.',
                  'Click a <b>voice strip</b> to edit that voice. Click the coloured LED next to its name to hear it.',
                  'Click <b>RANDOM</b> for a new kit, <b>UNDO</b> to go back.',
                  'Play the voices from a keyboard or MIDI clip: C1–F#1 (notes 36–42).'])

    s += [Paragraph('3. The panel', h2)]
    s += [Paragraph('Top bar', h3)]
    s += [table([['Control', 'What it does'],
                 ['LX-6 / gumroad link', 'Opens the Ticco Ross store.'],
                 ['KIT', 'Shows the kit name. Click for the kit menu: Init Kit, your saved kits, Save kit as…, Load kit file…, Show kits folder.'],
                 ['SAVE', 'Saves the current sound as a .lx6kit file (default folder: Documents/LX-6/Kits).'],
                 ['RANDOM · AMT · UNDO', 'Randomizes the whole kit. AMT sets how far: low = variations of the current kit, full = brand new sounds. '
                                         'Locked sections are kept. UNDO steps back (up to 32 times).'],
                 ['TEMPO', 'Shows the host tempo (HOST SYNC) or the internal tempo (INTERNAL). Drag up/down to change the internal tempo.'],
                 ['MASTER', 'Output level (−60 to +6 dB). Double-click: 0 dB. The meter shows the output.'],
                 ['LXR', 'Hardware editor settings for an Erica Synths LXR-02 (lit when active). See chapter 7.'],
                 ['i', 'About LX-6.']], [42 * mm, W - 42 * mm])]
    s += [Paragraph('Voice strips', h3),
          Paragraph('Six strips, one per voice: DRUM 1, DRUM 2, DRUM 3, SNARE, CYMBAL, HI-HAT. Each has a trigger LED (click to audition), '
                    'an envelope picture (attack, decay, slope), COARSE pitch, DECAY, PAN, a volume fader and M (mute) / S (solo). '
                    'The selected strip is outlined; its voice is shown in the editor. Double-click any knob to reset it to the Init kit value.', body)]
    s += [Paragraph('Voice editor', h3),
          Paragraph('Two pages for the selected voice. Each section has a dice (randomize just this section) and a lock '
                    '(keep this section when you randomize the kit or the voice). The dice in the header randomizes the whole voice. '
                    'Hover a knob to see its value.', body)]
    s += [table([['Page / section', 'Parameters'],
                 ['SOUND · OSCILLATOR', 'Wave (sine, triangle, saw, square, noise, PWM), coarse, fine, PWM. Drums: modulation wave, FM amount, '
                                        'FM frequency, FM/Mix mode. Snare: noise frequency and noise mix. Cymbal and hi-hat: waves 2 and 3, '
                                        'two modulation oscillators (frequency and gain).'],
                 ['SOUND · PITCH MOD', 'Drums and snare: pitch envelope decay, slope and amount.'],
                 ['SOUND · TRANSIENT', 'Cymbal and hi-hat (in place of pitch mod): transient wave, level and frequency.'],
                 ['SOUND · FILTER', 'Type (LP, HP, BP, UBP, notch, peak, LP2, off), cutoff, resonance, filter drive.'],
                 ['SOUND · AMP ENVELOPE', 'Attack, decay (hi-hat: closed and open decay), slope. Snare and cymbal: repeat.'],
                 ['MOD · TRANSIENT', 'Drums and snare: transient wave, level and frequency.'],
                 ['MOD · VOICE', 'Volume, pan, drive, S.RATE (sample-rate reduction: 127 is clean, lower is crunchier), output '
                                 '(St1, St2, L1, R1, L2, R2 or FX bus). Locked by default so random kits keep their mix.'],
                 ['MOD · VELOCITY', 'Velocity amount, velocity destination (any parameter), velocity to volume on/off.']],
                [42 * mm, W - 42 * mm])]
    s += [Paragraph('FX bus and LFO (right column)', h3),
          Paragraph('Like on the LXR, the FX bus only processes voices that are <b>sent</b> to it. Use the <b>SEND VOICES TO FX</b> buttons '
                    '(D1 D2 D3 SN CY HH) to send a voice: its OUTPUT switches to FX and it is heard through the effect; click again to send it '
                    'back to the main output. Then choose OFF, DRV (drive), RNG (ring modulator), CMP (compressor) or DEL (delay).', body),
          Paragraph('The knob names follow the effect: delay = TIME, RANGE, TONE, FEEDBACK; compressor = ATTACK, RELEASE, THRESH, GAIN; '
                    'drive = DRIVE and three shaping controls; ring = FREQ. DRY/WET blends the sent voices between dry and effect. '
                    'Extra menus appear for the chosen effect (distortion type, ratio, ring wave, delay or ping-pong). FX OUT sets which '
                    'output the effect returns to. S.RATE ALL reduces the sample rate of everything and works without sending.', body),
          Paragraph('The LFO block belongs to the selected voice: RATE, DEPTH, OFFSET, SHAPE (sine, triangle, saw up/down, square, random, '
                    'exponential up/down), SYNC (off or 4/1 … 1/32 of the tempo), DEST (any parameter of any voice) and RETRIG (restart the '
                    'LFO when a voice plays).', body)]

    s += [Paragraph('4. Sequencer', h2)]
    s += bullets(['<b>A–H</b>: eight patterns. Click to play and edit a pattern; while the sequencer runs it switches at the end of the bar '
                  '(the button blinks until then). Shift-click: edit without switching. Alt-click: copy the pattern you are editing to that slot.',
                  '<b>D1 D2 D3 SN CY HH OH</b>: the track you are editing (OH = open hi-hat, which chokes the closed hi-hat). '
                  'Selecting a voice strip selects its track too.',
                  '<b>Steps 1–16</b>: click to switch steps on/off. Above each step is a lane: drag it to set the step\'s velocity '
                  '(<b>VEL</b>) or probability (<b>PROB</b>; steps below 100 % show their percentage).',
                  '<b>EUCLID</b>: spreads a number of pulses evenly over 16 steps, with rotation.',
                  '<b>RND</b>: random steps for this track (Shift: all tracks). <b>CLR</b>: clear the track. <b>COPY</b>: copy the pattern to the next slot.',
                  '<b>PLAY</b>: runs LX-6\'s own clock when the host is stopped (standalone).',
                  'Swing is a host-automatable parameter (Swing, 0–75 %). Mute/solo per voice are automatable too.'])

    s += [Paragraph('5. MIDI and automation', h2)]
    s += [table([['Note', 'Plays'], ['C1 (36)', 'Drum 1'], ['C#1 (37)', 'Drum 2'], ['D1 (38)', 'Drum 3'], ['D#1 (39)', 'Snare'],
                 ['E1 (40)', 'Cymbal'], ['F1 (41)', 'Closed hi-hat'], ['F#1 (42)', 'Open hi-hat']], [30 * mm, 60 * mm])]
    s += [Spacer(1, 4), Paragraph('Every sound parameter, master, swing, tempo and mute/solo are host parameters, named like '
                                  '"Drum 1 Cutoff" or "Snare P Decay". Velocity from notes and steps is used by the voices.', body)]

    s += [Paragraph('6. Outputs', h2),
          Paragraph('The LXR has two stereo outputs and LX-6 keeps them. Normally both are mixed to the main output. If you enable LX-6\'s '
                    '<b>Out 2</b> bus in your DAW, voices whose OUTPUT is St2/L2/R2 come out there instead, for separate processing. '
                    'L/R settings send a voice to one side only.', body)]

    s += [Paragraph('7. LXR-02 hardware editor', h2),
          Paragraph('If you own an Erica Synths LXR-02, LX-6 can edit and play it. Click <b>LXR</b> in the top bar:', body)]
    s += bullets(['<b>MODE</b>: OFF; <b>LXR ONLY</b> (LX-6\'s own engine is silent, you hear the hardware); <b>LX-6 + LXR</b> (both play).',
                  '<b>MIDI OUTPUT</b>: the port your LXR is connected to. With "None", messages only go to the plugin\'s MIDI output '
                  '(route it in your DAW if it supports plugin MIDI out).',
                  '<b>PARAMETER CH</b>: the channel for parameter changes. Set the LXR\'s global MIDI channel to the same number.',
                  '<b>Voice channels</b>: the channel each track plays on (default 1–7). Set each voice\'s MIDI channel on the LXR to match.',
                  '<b>SEND KIT TO LXR</b>: sends every parameter once, so the LXR sounds like LX-6.'])
    s += [Paragraph('Parameters 1–126 are sent as CC (CC number = parameter + 1), parameters 128 and up as NRPN (CC 99/98 + data entry CC 6). '
                    'LFO and velocity destinations can only be set on the LXR itself. Hardware mode was built from the LXR firmware\'s MIDI '
                    'code; it has not been tested on a physical LXR-02 yet.', small)]

    s += [Paragraph('8. Good to know', h2)]
    s += bullets(['<b>One engine per project.</b> The LXR engine is one piece of firmware, so only the first LX-6 in a project plays sound. '
                  'More LX-6 instances say so in their top bar and still work as sequencer + LXR editor. Delete the first one and the next takes over.',
                  'The engine runs at the LXR\'s own sample rate (44.14 kHz) and LX-6 converts to your project rate, so it sounds the same at 44.1, 48 or 96 kHz.',
                  'Kits are small XML files (.lx6kit) you can share. Your sequencer patterns are saved with the DAW project.',
                  'The window can be resized from the bottom-right corner.'])

    s += [Paragraph('9. Credits and licence', h2),
          Paragraph('LX-6 by Ticco Ross — ticcoross.gumroad.com — ticcoross@gmail.com.', body),
          Paragraph('Sound engine: Erica Synths LXR firmware, Copyright © 2012–2026 Erica Synths, GPL-3.0. "Erica Synths", "LXR" and "LXR-02" '
                    'are trademarks of Erica Synths, used here only to describe origin and compatibility.', body),
          Paragraph('LX-6 is free software under the GNU General Public License v3: you may use, study, change and share it. '
                    'It comes without any warranty. Source code: github.com/Spunkmeyer77/LX6. Built with JUCE. '
                    'Fonts: Space Grotesk and JetBrains Mono (SIL Open Font License).', body)]
    doc.build(s)


def history():
    doc = make_doc(os.path.join(DOCS, 'LX-6_Update_History.pdf'), 'Update History')
    W = doc.width
    s = [Paragraph('LX-6 Update History', h1), Paragraph('Newest version first.', small), Spacer(1, 6)]
    s += [Paragraph(f'Version 0.1.1  ·  {DATE}', h2), Paragraph('Fixes.', body)]
    s += bullets(['The KIT name field no longer disappears after touching the interface.',
                  'FX bus: new <b>SEND VOICES TO FX</b> buttons route voices into the effect (before, this was only possible through each '
                  'voice\'s OUTPUT setting and was easy to miss).',
                  'FX bus starts with audible settings (dry/wet 50, delay tone and feedback up) instead of the LXR init kit\'s zeros, '
                  'which left the delay silent.',
                  'FX knobs are labelled per effect (TIME / RANGE / TONE / FEEDBACK for the delay, ATTACK / RELEASE / THRESH / GAIN for the compressor).'])
    s += [Paragraph(f'Version 0.1.0  ·  {DATE}', h2), Paragraph('First release.', body)]
    s += bullets(['LXR firmware sound engine running as a plugin: 6 voices (Drum 1–3, Snare, Cymbal, Hi-hat with open/closed), '
                  'FX bus (drive, ring, compressor, delay/ping-pong), one LFO per voice, velocity modulation, two stereo outputs.',
                  'Formats: Audio Unit, VST3 and standalone for macOS (universal); VST3 and standalone for Windows (64-bit).',
                  'Black LX-6 interface: voice strips with envelope pictures, two-page voice editor, FX and LFO column, resizable window.',
                  '16-step sequencer: 7 tracks × 8 patterns, velocity and probability per step, Euclidean generator, random steps, '
                  'copy, swing, host sync and an internal clock.',
                  'Randomizer for kit, voice or section, with amount, section locks and undo; random kits are kept audible.',
                  'LXR-02 hardware editor mode: parameters as CC/NRPN, sequencer and notes on per-voice channels, send-kit button.',
                  'Kits saved and loaded as .lx6kit files; patterns and settings saved with the project.',
                  'Custom About window with store, contact and source links.',
                  'Engine fixes for desktop use: safe wavetable, distortion and delay-buffer access (the hardware relied on reading neighbouring memory).'])
    doc.build(s)


if __name__ == '__main__':
    manual()
    history()
    print('docs written')
