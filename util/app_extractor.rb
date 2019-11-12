#!/usr/bin/env ruby
#
# Copyright (c) 2019 joshua stein <jcs@jcs.org>
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

#
# Extract an application from a Mailstation dataflash dump including its
# icons/screens and program code.
#
# Usage: app_extractor.rb -d <path to dataflash> -a <app 0-4>
#
# Assemble output with:
#  ruby app_extractor.rb -f dataflash.bin -a 0 > app.asm
#  sdasz80 -l app.lst app.asm
#
# Then compare the byte listing in app.lst with the hexdump output of
# dataflash.bin and they should be identical.
#

require "getoptlong"

ORG = 0x4000

opts = GetoptLong.new(
  [ "--file", "-f", GetoptLong::REQUIRED_ARGUMENT ],
  [ "--app", "-a", GetoptLong::REQUIRED_ARGUMENT ],
)

dataflash = nil
app = -1

opts.each do |opt,arg|
  case opt
  when "--file"
    dataflash = arg

  when "--app"
    app = arg.to_i
  end
end

if !dataflash || app < 0 || app > 4
  usage
end

@file = File.open(dataflash, "rb")
@file.seek(ORG * app)

if (b = read(1)) != 0xc3
  raise "expected jp, got #{b.inspect}"
end

puts "\t.module\tapp#{app}"
puts ""
puts "\t.area\t_DATA"
puts "\t.area\t_HEADER (ABS)"
puts "\t.org\t#{sprintf("0x%04x", ORG)}"
puts ""

code_base = read(2)
puts "\tjp\t#{x code_base, 4}"

icon_base = read(2)
puts "\t.dw\t(icons)\t\t; #{x icon_base, 4}"
puts "\t.dw\t(caption)\t; #{x read(2), 4}"
puts "\t.dw\t(dunno)\t\t; #{x read(2), 4}"

puts "dunno:"
puts "\t.db\t##{x read(1), 2}"

puts "xpos:"
puts "\t.dw\t##{x read(2), 4}"
puts "ypos:"
puts "\t.dw\t##{x read(2), 4}"

puts "caption:"
puts "\t.dw\t##{x read(2), 4}"
puts "\t.dw\t(endcap - caption - 6) ; calc caption len (##{x read(2), 4})"
puts "\t.dw\t##{x read(2), 4}\t\t; offset to first char"

print "\t.ascii\t\""
while true do
  z = read(1)
  if z == 0
    break
  end

  print z.chr
end

puts "\""
puts "endcap:"

puts ""
puts "\t.org\t#{x icon_base, 4}"
puts ""

@file.seek((ORG * app) - ORG + icon_base)

puts "icons:"

icons = [ {}, {} ]

2.times do |x|
  icons[x][:size] = read(2)
  puts "\t.dw\t##{x icons[x][:size], 4}\t\t; size icon#{x}"
  icons[x][:pos] = read(2)
  puts "\t.dw\t(icon#{x} - icons)\t; offset to icon#{x} (#{x icons[x][:pos], 4})"
end

2.times do |i|
  puts "icon#{i}:"

  icons[i][:width] = read(2)
  puts "\t.dw\t##{x icons[i][:width], 4}\t\t; icon width (#{icons[i][:width]})"
  icons[i][:height] = read(1)
  puts "\t.db\t##{x icons[i][:height], 2}\t\t; icon height " <<
    "(#{icons[i][:height]})"

  puts ""

  row = []
  icons[i][:cols] = (icons[i][:width] / 8.0).ceil

  (icons[i][:size] - 3).times do |j|
    row.push read(1)

    if row.count == icons[i][:cols]
      # each byte is stored in memory in order, but each byte's bits are drawn
      # on the screen right-to-left
      puts "\t.db\t" + row.map{|z| "##{x(z, 2)}" }.join(", ") +
        "\t; " + row.map{|z| sprintf("%08b", z).reverse }.join.
        gsub("0", ".").gsub("1", "#")
      row = []
    end
  end

  puts ""
end

@file.seek((ORG * app) - ORG + code_base)

while !@file.eof?
  o = [ read(1) ]

  if o[0] == 0
    fp = @file.pos
    if @file.read(10) == ("\0" * 10)
      # assume a long string of nops is the end of the line
      break
    else
      # put those nops back
      @file.seek(fp)
    end
  end

  l = oclen(o[0])
  if l > 1
    o += (l - 1).times.map{ read(1) }
  end

  puts oc(o)
end

BEGIN {
  def read(l)
    d = @file.read(l)
    if l == 1
      d.unpack("C*")[0]
    elsif l == 2
      d.unpack("v*")[0]
    end
  end

  def x(v, l = 1)
    sprintf("0x%0#{l}x", v)
  end

  def usage
    puts "usage: #{$0} -f <path to dataflash.bin> -a <app number 0-4>"
    exit 1
  end

  @opcodes = {}
  File.open("z80_opcodes.txt") do |f|
    while f && !f.eof?
      oc, desc = f.gets.strip.split(/  +/, 2)

      t = oc.split(" ")
      @opcodes[t[0].to_i(16)] = {
        :desc => desc,
        :length => t.count,
        :format => t,
      }
    end
  end

  def oclen(c)
    return @opcodes[c][:length]
  end

  def oc(c_a)
    # ld de, nn
    # ld (nn), hl
    # call nn
    # jr z, e
    # ld b, n
    # rlc b
    # ret

    op = @opcodes[c_a[0]]

    desc = op[:desc].split(" ").map.with_index{|c,x|
      if c == "nn" || c == "(nn)"
        # use two bytes of c_a
        i = (c_a[1].chr + c_a[2].chr).unpack("v*")[0]

        if op[:desc][0, 2] == "jp"
          # jp takes an address, not a number
          c = sprintf("0x%04x", i)
        elsif c == "(nn)"
          c = sprintf("(#0x%04x)", i)
        else
          c = sprintf("#0x%04x", i)
        end

      elsif c == "n"
        c = sprintf("#0x%02x", c_a[1])

      elsif c == "e" && op[:format][1] == "xx" # e when xx not in opcode is register e
        if op[:desc][0, 2] == "jr"
          c = c_a[1]
        else
          # calculate relative offset
          c = sprintf("0x%04x", (@file.pos + c_a[1]))
        end
      end

      c
    }.join(" ")

    "\t#{desc}#{desc.length < 8 ? "\t" : ""}#{desc.length < 16 ? "\t" : ""}\t;" +
      c_a.map{|b| sprintf(" %02x", b) }.join
  end
}
