tab = ' '*4

types = {
    'Unorm' => 'CCColourFormatSpaceHS_HSV | CCColourFormatTypeUnsignedInteger | CCColourFormatNormalized',
    'Snorm' => 'CCColourFormatSpaceHS_HSV | CCColourFormatTypeSignedInteger | CCColourFormatNormalized',
    'Uint' => 'CCColourFormatSpaceHS_HSV | CCColourFormatTypeUnsignedInteger',
    'Sint' => 'CCColourFormatSpaceHS_HSV | CCColourFormatTypeSignedInteger',
    'Float' => 'CCColourFormatSpaceHS_HSV | CCColourFormatTypeFloat'
}

names = [
    'HSV32Uint',
    'HSV32Sint',
    'HSV32Float',
    'HSVA32Uint',
    'HSVA32Sint',
    'HSVA32Float'
]


def colour_format_channel_type(channel, bits, offset, space)
    pad = space - channel.length
    "CC_COLOUR_FORMAT_CHANNEL(CCColourFormatChannel#{channel}, #{' '*(pad < 0 ? 0 : pad)}#{bits}, CCColourFormatChannelOffset#{offset})"
end

enums = ''
bc = 0
chan = ''
names.each { |name|
    type = name[/(?<=\d)+\D+\z/]

    definition = tab + 'CCColourFormat' + name

    c = definition.length % tab.length
    odd = ((definition.length + (tab.length - c)) / tab.length) % 2
    t = (tab * (1 + odd))
    t = t[0...(t.length - c)]

    cn = name.chomp(type)
    if cn[/8/] and (tab + cn).length % tab.length == 1
        t << tab
    end

    definition << t + '= '

    c = (definition.length + 2) / tab.length

    definition << types[type] + "\n"

    cs = 10

    b = 0
    index = 0
    name.scan(/\D+\d+/).each { |channel_size|
        bits = channel_size[/\d+/]
        channel_size.scan(/\D/).map { |channel|
            b += bits.to_i
            definition << tab*c + '| ' + colour_format_channel_type({ 'H' => 'Hue', 'S' => 'Saturation', 'V' => 'Value', 'A' => 'Alpha' }[channel], bits, index, cs) + "\n"
            index += 1
        }
    }

    if b != bc
        enums << "\n\n"*(bc > 0 ? 1 : 0)
        enums << tab + "//#{b}-bit\n"
        bc = b
        chan = ''
    end

    if chan != cn
        enums << "\n\n"*(chan.length > 0 ? 1 : 0)
        enums << tab + "//#{cn}\n"
        chan = cn
    end

    enums << definition.chomp + ",\n"
}


src = """
//This file is automatically generated, modifications will be lost!
typedef enum {
#{enums.chomp}
} CCColourFormatHSV;
""".lstrip

File.write('src/rendering/utilities/colour/ColourFormatHSV.h', src)
