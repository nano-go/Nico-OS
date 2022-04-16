#!/usr/bin/ruby

# This script generates the "kernel/syscall.h" and the "lib/src/syscall.asm"

# "kernel/syscall.h" contains syscall numbers.
# "lib/src/syscall.asm" contains syscall entry code.

def entry(name, nparams)
  return {
    name: name,
    nparams: nparams
  }
end

SYSCALLS = [
  entry("getpid", 0),
  entry("write", 3),
  entry("read", 3),
  entry("open", 2),
  entry("stat", 2),
  entry("close", 1),
  entry("mkdir", 1),
  entry("unlink", 1),
  entry("yield", 0),
  entry("fork", 0),
  entry("sbrk", 1),
  entry("execv", 2),
  entry("exit", 1),
  entry("wait", 1),
  entry("chdir", 1),
  entry("dup", 1),
  entry("pipe", 1),
]

def gen_syscall_h file
  content = [
    "#ifndef _KERNEL_SYSCALL_H
#define _KERNEL_SYSCALL_H
"
  ]
  SYSCALLS.each_with_index do |element, index|
    content.append "#define SYS_#{element[:name]} #{index}"
  end
  content.append "\n#endif /* _KERNEL_SYSCALL_H */"
  File.write(file, content.join("\n"))
end

def gen_syscall_asm_code entry, func_num
  registers = ["ebx", "ecx", "edx"]
  content = []
  content.append "\tmov eax, #{func_num}"
  for i in 0...(entry[:nparams])
    content.append "\tmov #{registers[i]}, [esp + #{4 * (i + 1)}]"
  end
  content.append "\tint 0x80"
  content.append "\tret"
  return content.join "\n"
end

def gen_syscall_asm file
  content = []
  SYSCALLS.each_with_index do |element, index|
    content.append "section .text"
    content.append "global #{element[:name]}"
    content.append "$#{element[:name]}: "
    content.append gen_syscall_asm_code(element, index)
    content.append ""
  end
  File.write file, content.join("\n")
end

gen_syscall_h "./include/kernel/syscall.h"
gen_syscall_asm "./lib/src/syscall.asm"
