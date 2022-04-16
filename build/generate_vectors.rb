# This script is used to generate the table of the interrupt
# entry code.

ERRORCODE_VECS = [8, 17] + (10..14).to_a

puts "[bits 32]"
puts "extern alltraps_entry"
for i in 0..255
  puts "section .text"
  puts "intr_vec_#{i}:"
  if not ERRORCODE_VECS.include? i
    puts "    push 0"
  end
  puts "    push #{i}"
  puts "    jmp alltraps_entry"
end

puts "section .data"
puts "global vectors"
puts "vectors:"
for i in 0..255
  puts "dd intr_vec_#{i}"
end
