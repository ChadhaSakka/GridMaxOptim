# TP2 AOC : Optimisation d’un Générateur de Grille 2D

**Auteur : Chadha Sakka**  
**Date : 24 Février 2025**

## Table des matières
1. [Introduction](#introduction)
2. [Environnement d’Exécution](#environnement-dexécution)
   - 2.1 [Configuration matérielle et logicielle](#configuration-matérielle-et-logicielle)
   - 2.2 [Étapes préliminaires](#étapes-préliminaires)
3. [Analyse de la Version Initiale (Baseline)](#analyse-de-la-version-initiale-baseline)
   - 3.1 [Analyse du code](#analyse-du-code)
   - 3.2 [Mesures de performances avec MAQAO](#mesures-de-performances-avec-maqao)
   - 3.3 [Interprétation](#interprétation)
4. [Optimisations Appliquées](#optimisations-appliquées)
   - 4.1 [Optimisation 1 : Compilation avec options d’optimisation](#optimisation-1--compilation-avec-options-doptimisation)
   - 4.2 [Optimisation 2 : Suppression de qsort et recherche linéaire](#optimisation-2--suppression-de-qsort-et-recherche-linéaire)
   - 4.3 [Optimisation 3 : Génération de valeurs aléatoires avec buffering](#optimisation-3--génération-de-valeurs-aléatoires-avec-buffering)
   - 4.4 [Optimisation 4 : Remplacement du tableau de pointeurs par un tableau plat](#optimisation-4--remplacement-du-tableau-de-pointeurs-par-un-tableau-plat)
   - 4.5 [Optimisation 5 : Vectorisation AVX2 de find_max_v2](#optimisation-5--vectorisation-avx2-de-find_max_v2)
5. [Visualisation des Performances avec MAQAO](#visualisation-des-performances-avec-maqao)
6. [Conclusion](#conclusion)

---

## Introduction

Imaginez un programme qui génère une grille 2D remplie de nombres aléatoires, puis identifie les valeurs maximales (`v1` et `v2`) tout en localisant leurs positions. Ce rapport détaille l’optimisation de ce code initial, transformant une version lente en une version performante, en explorant ses rouages et en appliquant des techniques avancées. Nous analysons les performances avec MAQAO, en comparant la version initiale (Baseline) et la version optimisée, avec des mesures pour 3 et 10 répétitions sur une grille de 2000×3000 points.

---

## Environnement d’Exécution

### Configuration matérielle et logicielle

| Caractéristique      | Détails                      |
|----------------------|------------------------------|
| Processeur           | Intel Core i5-8250U @ 1.60 GHz |
| Cœurs/Threads        | 4 cœurs, 8 threads           |
| Mémoire              | 8 GB DDR4 @ 2400 MT/s        |
| OS                   | Ubuntu 22.04, Noyau 5.15     |
| Compilateur          | Clang 2024                   |

### Étapes préliminaires

Avant chaque mesure, nous avons :
- Régulé la charge système avec :  
  ```bash
  sudo cpupower frequency-set -g performance
