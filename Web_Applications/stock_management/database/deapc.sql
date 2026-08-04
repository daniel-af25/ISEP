-- phpMyAdmin SQL Dump
-- version 5.2.1
-- https://www.phpmyadmin.net/
--
-- Host: 127.0.0.1
-- Tempo de geração: 21-Ago-2025 às 19:08
-- Versão do servidor: 10.4.32-MariaDB
-- versão do PHP: 8.0.30

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
START TRANSACTION;
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;

--
-- Banco de dados: `deapc`
--

-- --------------------------------------------------------

--
-- Estrutura da tabela `clients`
--

CREATE TABLE `clients` (
  `nif` int(9) NOT NULL,
  `name` varchar(20) DEFAULT NULL,
  `email` varchar(35) NOT NULL,
  `address` varchar(100) DEFAULT NULL,
  `payment` varchar(30) DEFAULT NULL,
  `phone` int(9) DEFAULT NULL,
  `date` date DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

--
-- Extraindo dados da tabela `clients`
--

INSERT INTO `clients` (`nif`, `name`, `email`, `address`, `payment`, `phone`, `date`) VALUES
(252002177, 'João Cardoso', '122564@isep.ipp.pt', 'Espinho', 'Prompt Payment', 912561788, '2025-06-20'),
(252002466, 'Daniel Afonso', 'danielkyyr@gmail.com', 'Travessa Fernão Magalhães, 143', 'Prompt Payment', 919099211, '2025-06-17'),
(252002478, 'Daniela Afonso', 'danielaafonso29@gmail.com', 'Travessa Fernão Magalhães, 144', 'Prompt Payment', 2147483647, '2025-06-17'),
(501123456, 'Ana Silva', 'ana.silva@example.com', 'Rua das Flores 123, Lisboa', 'Prompt Payment', 912345678, '2025-05-10'),
(502234567, 'Carlos Pereira', 'carlos.p@example.com', 'Avenida da Boavista 45, Porto', '30 Days', 923456789, '2025-05-21'),
(503345678, 'Mariana Costa', 'm.costa@email.net', 'Praceta da Alegria 8, Coimbra', 'Credit 180 days', 934567890, '2025-06-02'),
(504456789, 'João Rodrigues', 'j.rodrigues@email.org', 'Largo do Carmo 7, Faro', '30 Days', 965678901, '2025-06-11'),
(505567890, 'Sofia Almeida', 'sofia.a@example.com', 'Rua de Santa Catarina 99, Braga', 'Prompt Payment', 911223344, '2025-06-17'),
(506112233, 'Rui Martins', 'rui.martins@example.com', 'Rua de Cedofeita 600, Porto', '30 Days', 910112233, '2025-05-20'),
(507223344, 'Catarina Gomes', 'catarina.g@email.net', 'Avenida da República 101, Vila Nova de Gaia', 'Prompt Payment', 921223344, '2025-05-22'),
(508334455, 'Tiago Ferreira', 'tiago.f.88@example.com', 'Rua Brito Capelo 80, Matosinhos', 'Credit 180 days', 932334455, '2025-05-25'),
(509445566, 'Beatriz Santos', 'b.santos@email.org', 'Rua Dom Afonso Henriques 1100, Gondomar', '30 Days', 963445566, '2025-05-28'),
(510556677, 'Diogo Oliveira', 'diogo.oliveira@example.com', 'Largo da Estação 5, Valongo', 'Prompt Payment', 914556677, '2025-06-01'),
(511667788, 'Inês Pereira', 'ines.pereira@email.net', 'Avenida Mário Brito 20, Maia', '30 Days', 925667788, '2025-06-03'),
(512778899, 'Pedro Sousa', 'pedro.sousa91@example.com', 'Rua da Junqueira 45, Vila do Conde', 'Credit 180 days', 936778899, '2025-06-05'),
(513889900, 'Marta Fernandes', 'marta.fernandes@email.org', 'Alameda da Europa 7, Paços de Ferreira', 'Prompt Payment', 967889900, '2025-06-07'),
(514990011, 'José Correia', 'jose.c@example.com', 'Rua Nova de Fânzeres 33, Fânzeres', '30 Days', 918990011, '2025-06-09'),
(515001122, 'Laura Teixeira', 'laura.t@email.net', 'Praça da Liberdade 1, Santo Tirso', 'Prompt Payment', 929001122, '2025-06-11'),
(516112233, 'André Alves', 'andre.alves@example.com', 'Rua 25 de Abril 9, Paredes', 'Credit 180 days', 930112233, '2025-06-13'),
(517223344, 'Filipa Azevedo', 'filipa.azevedo@email.org', 'Avenida dos Descobrimentos 500, Penafiel', '30 Days', 961223344, '2025-06-14'),
(518334455, 'Gonçalo Rocha', 'goncalo.r@example.com', 'Rua dos Bombeiros Voluntários 12, Lousada', 'Prompt Payment', 912334455, '2025-06-15'),
(519445566, 'Daniela Ribeiro', 'd.ribeiro@email.net', 'Largo do Pelourinho 1, Amarante', '30 Days', 923445566, '2025-06-16'),
(520556677, 'Francisco Barbosa', 'f.barbosa.dev@example.com', 'Avenida Dr. Mário Soares 140, Trofa', 'Credit 180 days', 934556677, '2025-06-17');

-- --------------------------------------------------------

--
-- Estrutura da tabela `orders_client`
--

CREATE TABLE `orders_client` (
  `orderID` varchar(20) NOT NULL,
  `client_nif` int(11) DEFAULT NULL,
  `client_name` varchar(20) DEFAULT NULL,
  `stat` varchar(10) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

--
-- Extraindo dados da tabela `orders_client`
--

INSERT INTO `orders_client` (`orderID`, `client_nif`, `client_name`, `stat`) VALUES
('030558', 252002466, 'Daniel Afonso', 'SENT'),
('341BF2', 252002466, 'Daniel Afonso', 'NEW'),
('476C2C', 252002466, 'Daniel Afonso', 'SENT'),
('4A7158', 252002466, 'Daniel Afonso', 'SENT'),
('5BABA6', 501123456, 'Ana Silva', 'SENT'),
('6865A2', 252002466, 'Daniel Afonso', 'SENT'),
('961108', 252002466, 'Daniel Afonso', ' SENT'),
('9A4DE1', 252002177, 'João Cardoso', 'NEW'),
('9B7A85', 252002466, 'Daniel Afonso', 'SENT'),
('ACA6A1', 252002466, 'Daniel Afonso', ' CANCELLED'),
('F14A12', 507223344, 'Catarina Gomes', ' CANCELLED'),
('FC7ABC', 252002478, 'Daniela Afonso', 'SENT');

-- --------------------------------------------------------

--
-- Estrutura da tabela `orders_products`
--

CREATE TABLE `orders_products` (
  `orderID` varchar(20) DEFAULT NULL,
  `prodID` int(11) DEFAULT NULL,
  `prod_name` varchar(20) DEFAULT NULL,
  `prod_desc` varchar(200) DEFAULT NULL,
  `qty` int(11) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

--
-- Extraindo dados da tabela `orders_products`
--

INSERT INTO `orders_products` (`orderID`, `prodID`, `prod_name`, `prod_desc`, `qty`) VALUES
('5BABA6', 8, 'Samsung Galaxy S24', 'Electronics', 2),
('FC7ABC', 15, '4K Monitor 27-inch', 'Electronics', 1),
('6865A2', 8, 'Samsung Galaxy S24', 'Electronics', 2),
('6865A2', 10, 'Wireless Mouse', 'Electronics', 1),
('F14A12', 17, 'Document Shredder', 'Office Supplies', 20),
('9A4DE1', 8, 'Samsung Galaxy S24', 'Electronics', 2),
('961108', 8, 'Samsung Galaxy S24', 'Electronics', 500),
('476C2C', 10, 'Wireless Mouse', 'Electronics', 1),
('4A7158', 19, 'Coffee Machine', 'Other', 1),
('ACA6A1', 10, 'Wireless Mouse', 'Electronics', 500),
('341BF2', 8, 'Samsung Galaxy S24', 'Electronics', 200000),
('341BF2', 7, 'Aspirador', 'Eletrodoméstico', 1),
('030558', 13, 'A4 Printer Paper 500', 'Office Supplies', 1),
('9B7A85', 8, 'Samsung Galaxy S24', 'Electronics', 2);

-- --------------------------------------------------------

--
-- Estrutura da tabela `products`
--

CREATE TABLE `products` (
  `id` int(11) NOT NULL,
  `name` varchar(20) DEFAULT NULL,
  `supplier` varchar(20) DEFAULT NULL,
  `price` float DEFAULT NULL,
  `category` varchar(50) DEFAULT NULL,
  `notes` varchar(50) DEFAULT NULL,
  `date` date DEFAULT NULL,
  `qty` int(11) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

--
-- Extraindo dados da tabela `products`
--

INSERT INTO `products` (`id`, `name`, `supplier`, `price`, `category`, `notes`, `date`, `qty`) VALUES
(7, 'Aspirador', 'Worten', 50, 'Eletrodoméstico', 'Fixe', '2025-06-07', 15),
(8, 'Samsung Galaxy S24', 'Samsung', 455, 'Electronics', 'NOVOS', '2025-06-17', 14),
(10, 'Wireless Mouse', 'Logitech', 20, 'Electronics', 'New stock arrived', '2025-06-15', 500),
(11, 'Ergonomic Office Cha', 'Staples', 180, 'Furniture', 'Assembly required', '2025-06-16', 10),
(12, 'Antivirus Pro 1-Year', 'Norton', 45, 'Software', 'Digital license key', '2025-06-17', 200),
(13, 'A4 Printer Paper 500', 'HP', 8, 'Office Supplies', 'On sale this week', '2025-06-14', 149),
(14, 'Cleaning Spray', 'CleanCo', 5, 'Other', 'Multi-purpose', '2025-06-12', 75),
(15, '4K Monitor 27-inch', 'Dell', 299, 'Electronics', 'HDMI and DisplayPort', '2025-06-17', 24),
(16, 'Standing Desk Conver', 'FlexiSpot', 150, 'Furniture', 'Black model', '2025-06-16', 15),
(17, 'Document Shredder', 'AmazonBasics', 40, 'Office Supplies', '8-sheet capacity', '2025-06-16', 50),
(18, 'Cloud Storage 1TB Pl', 'Google', 99, 'Software', 'Billed annually', '2025-06-17', 200),
(19, 'Coffee Machine', 'Nespresso', 85, 'Other', 'Capsule system', '2025-06-15', 21),
(20, 'Laptop Power Adapter', 'HP', 55, 'Electronics', 'Check compatibility', '2025-06-14', 40),
(21, 'Bookshelf 5-Tier', 'IKEA', 70, 'Furniture', 'Pine wood finish', '2025-06-13', 18),
(22, 'Box of 100 Ballpoint', 'BIC', 12, 'Office Supplies', 'Blue ink', '2025-06-12', 150),
(23, 'Graphic Design Suite', 'Adobe', 599, 'Software', '1-year subscription', '2025-06-11', 50),
(24, 'Bluetooth Speaker', 'JBL', 65, 'Electronics', 'Waterproof model', '2025-06-10', 60),
(25, 'Filing Cabinet 3-Dra', 'Bisley', 210, 'Furniture', 'Steel, grey color', '2025-06-09', 12),
(26, 'Sticky Notes Variety', 'Post-it', 9, 'Office Supplies', 'Assorted colors', '2025-06-08', 300),
(27, 'Cybersecurity Packag', 'McAfee', 79, 'Software', '5-device license', '2025-06-07', 80),
(28, 'Wall Clock', 'Seiko', 30, 'Other', 'Silent sweep movement', '2025-06-06', 45),
(29, 'Mechanical Keyboard', 'Razer', 130, 'Electronics', 'RGB lighting', '2025-06-05', 35);

-- --------------------------------------------------------

--
-- Estrutura da tabela `users`
--

CREATE TABLE `users` (
  `username` varchar(20) NOT NULL,
  `email` varchar(30) DEFAULT NULL,
  `passw` varchar(20) DEFAULT NULL,
  `adm` tinyint(1) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

--
-- Extraindo dados da tabela `users`
--

INSERT INTO `users` (`username`, `email`, `passw`, `adm`) VALUES
('Arthur Peixoto', '1192117@isep.ipp.pt', '4cd79461', 1),
('Daniel Afonso', '1240570@isep.ipp.pt', '2525', 1),
('Daniel Afonso2', 'danielkyyr@gmail.com', 'dba87c10', 0),
('Daniel Afonso3', 'danielkyyr@gmail.com', '6e861945', 0),
('Marcel Silva', '1240575@isep.ipp.pt', 'c864bb3c', 1),
('Vitor Hugo', 'vitorpereira109@gmail.com', '7950b1fd', 1);

--
-- Acionadores `users`
--
DELIMITER $$
CREATE TRIGGER `generate_password` BEFORE INSERT ON `users` FOR EACH ROW SET NEW.passw = SUBSTRING(MD5(RAND()), 1, 8)
$$
DELIMITER ;

--
-- Índices para tabelas despejadas
--

--
-- Índices para tabela `clients`
--
ALTER TABLE `clients`
  ADD PRIMARY KEY (`nif`);

--
-- Índices para tabela `orders_client`
--
ALTER TABLE `orders_client`
  ADD PRIMARY KEY (`orderID`),
  ADD KEY `client_nif` (`client_nif`);

--
-- Índices para tabela `orders_products`
--
ALTER TABLE `orders_products`
  ADD KEY `orderID` (`orderID`),
  ADD KEY `prodID` (`prodID`);

--
-- Índices para tabela `products`
--
ALTER TABLE `products`
  ADD PRIMARY KEY (`id`);

--
-- Índices para tabela `users`
--
ALTER TABLE `users`
  ADD PRIMARY KEY (`username`);

--
-- AUTO_INCREMENT de tabelas despejadas
--

--
-- AUTO_INCREMENT de tabela `products`
--
ALTER TABLE `products`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=31;

--
-- Restrições para despejos de tabelas
--

--
-- Limitadores para a tabela `orders_client`
--
ALTER TABLE `orders_client`
  ADD CONSTRAINT `orders_client_ibfk_1` FOREIGN KEY (`client_nif`) REFERENCES `clients` (`nif`);

--
-- Limitadores para a tabela `orders_products`
--
ALTER TABLE `orders_products`
  ADD CONSTRAINT `orders_products_ibfk_1` FOREIGN KEY (`orderID`) REFERENCES `orders_client` (`orderID`),
  ADD CONSTRAINT `orders_products_ibfk_2` FOREIGN KEY (`prodID`) REFERENCES `products` (`id`);
COMMIT;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
