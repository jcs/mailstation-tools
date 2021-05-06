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
# Convert a 1-bit XPM to a Mailstation icon data structure
#
# Usage: xpm_to_icon.rb -f <xpm file>
#

require "getoptlong"

opts = GetoptLong.new(
  [ "--file", "-f", GetoptLong::REQUIRED_ARGUMENT ],
)

icon = nil
opts.each do |opt,arg|
  case opt
  when "--file"
    icon = arg
  end
end

if !icon
  puts "usage: #{$0} -f <xpm file>"
  exit 1
end

xpm = File.open(icon, "r")
if !(line = xpm.gets).match(/^\/\* XPM/)
  raise "unexpected line: #{line.inspect}"
end

if !(line = xpm.gets).match(/^static char/)
  raise "unexpected line: #{line.inspect}"
end

if !(m = (line = xpm.gets).match(/^\"(\d+) (\d+) /))
  raise "unexpected line: #{line.inspect}"
end
width = m[1].to_i
height = m[2].to_i

if width != 34 || height != 34
  raise "image should be 34x34, not #{width}x#{height}"
end

blank = nil
black = nil
2.times do
  line = xpm.gets
  if line.match(/^\" \tc None/)
    # skip
    line = xpm.gets
  end

  if !(m = line.match(/^\"(.)\tc #(FFFFFF|000000)/))
    raise "unexpected line: #{line.inspect}"
  end
  if m[2][0] == "F"
    blank = m[1]
  elsif m[2][0] == "0"
    black = m[1]
  else
    raise "unexpected color: #{line.inspect}"
  end
end

if !blank
  raise "no blank color found"
end
if !black
  raise "no black color found"
end

puts "icon0:"
puts "\t.dw\t##{sprintf("0x%02x", width)}\t\t\t; icon width (#{width})"
puts "\t.db\t##{sprintf("0x%02x", height)}\t\t\t; icon height (#{height})"
puts ""

while !xpm.eof?
  if !(m = (line = xpm.gets).match(/^\"(.{#{width}})"[,\}]/))
    raise "unexpected data line: #{line.inspect}"
  end

  if m[1].length != width
    raise "unexpected line length: #{m[1].length} != #{width}"
  end

  bits = m[1].split("").map{|b| b == black ? 1 : 0 }
  dbs = bits.each_slice(8).map{|byte| byte.reverse.join("").to_i(2) }

  puts "\t.db\t" << dbs.map{|i| sprintf("#0x%02x", i) }.join(", ") <<
    "\t; " << bits.map{|b| b == 1 ? "#" : "." }.join("")
end
