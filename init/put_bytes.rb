f=File.open("./initcode.bin", "r")
str=[]
f.each_byte { |ch|
  str.append("0x#{ch.to_s 16}")
}
puts str.join(", ")
