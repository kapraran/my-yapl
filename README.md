<table align="center">
<tr>
  <td align="center" width="9999">
  <img src="./assets/kapraran-yapl-logo.png" alt="my-yapl" width="128">

  # my-yapl
  </td>
</tr>
</table>

This is a complete compiler for a custom C-like programming language, created for educational purposes. It generates assembly code for the MIX architecture and is compatible with any emulator that supports it.

Check the examples directory to take an idea of yapl's syntax or refer to [this document](https://github.com/pahihu/mixal/blob/master/MIX.DOC) to find more information about MIX.

## Example

This is how the yapl language looks like
```
int main() {
    int a = 2, b;
    int c, d = a;
    return a + d;
}
```

And this is the resulting MIX assembly
```
ARSTACK	EQU	0
EXSTACK	EQU	150
V1D1101	EQU	796
V1C1100	EQU	797
V1B199	EQU	798
V1A198	EQU	799
PROGRAM	EQU	800
	ORIG	PROGRAM
	LDA	=2=
	STA	V1A198
	LDA	V1A198
	STA	V1D1101
	LDA	V1A198
	ADD	V1D1101
	JOV	ERROR
	STA	EXSTACK,5
	INC5	1
MM4425X	NOP	
	ENTA	0
	HLT	
ERROR	NOP	
	ENTA	1
	HLT	
	END	PROGRAM
```
## Build

I've also developed the utility script build.js to streamline the building process, offering additional features such as improved error highlighting and AST tree visualization.
