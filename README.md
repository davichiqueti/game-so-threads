# Shooter Simulation Game

## Visão Geral

Este é um jogo simples de simulação onde você controla um helicóptero para resgatar soldados enquanto evita tiros e obstáculos. Leve todos os soldados até a bandeira 

- **Helicóptero:** Controlado pelo jogador usando as setas do teclado.
- **Soldados:** Devem ser resgatados e levados até a bandeira.
- **Baterias Anti-Aereas:** Atiram automaticamente e precisam recarregar após alguns disparos.
- **Obstáculos:** Plataforma, caixas de munição e baterias anti-aereas que não podem ser tocadas.
- **Limites:** Se o avião voar alto demais o jogo acaba.

## Como Rodar

### 1. Instale as dependências

O jogo utiliza a biblioteca [SFML](https://www.sfml-dev.org/) para gráficos e entrada de teclado, além do compilador `g++`.  
A SFML é necessária para criar a janela do jogo, desenhar sprites e capturar eventos do teclado.

No terminal, execute:

```sh
make install
```

Isso irá instalar o `g++` e a biblioteca `libsfml-dev` no seu sistema (Ubuntu/Debian).

### 2. Compile o jogo

Compile o código-fonte com:

```sh
make compile
```

Isso irá gerar o executável `game.exe`.

Inicie o jogo com:

```sh
make run
# Ou usando o comando diretamente
./game.exe
```

## Controles

- **Setas do teclado:** Movem o helicóptero (cima, baixo, esquerda, direita).
