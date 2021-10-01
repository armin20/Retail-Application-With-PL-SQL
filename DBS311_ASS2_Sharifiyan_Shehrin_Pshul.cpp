#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <occi.h>
#include <string>
#include <numeric>

using oracle::occi::Environment;
using oracle::occi::Connection;

using namespace std;
using namespace oracle::occi;

string user = "dbs311_212za08";
string password = "82276420";
string connectionString = "myoracle12c.senecacollege.ca:1521/oracle12c";

struct ShoppingCart 
{
	int product_id;
	double price;
	int quantity;
};

int mainManu()
{
	int userInput = 0;
	auto checkIfBadValue = [](int value) { return cin.fail() || !(value <= 1 && value >= 0); };

	do
	{
		cout << "******************** Main Menu ********************" << endl;
		cout << "1)		Login" << endl;
		cout << "0)		Exit" << endl;
		if (checkIfBadValue(userInput)) cout << "You entered a wrong value. ";
		cout << "Enter an option (0-1): ";

		if (cin.fail())
		{
			cin.clear();
			cin.ignore(INT_MAX, '\n');
		}

		cin >> userInput;
	} while (checkIfBadValue(userInput));

	return userInput;
}

int getCustomerIdInput(Connection* conn)
{
	int userInput;

	do
	{		
		if (cin.fail())
		{
			cout << "Customer ID must be a number. ";
			cin.clear();
			cin.ignore(INT_MAX, '\n');
		}

		cout << "Enter the customer ID: ";
		cin >> userInput;
	} while (cin.fail());

	return userInput;
}

int customerLogin(Connection* conn, int customerId)
{
	Statement* statement = conn->createStatement("BEGIN find_customer(:1,:2); END;");
	statement->setNumber(1, customerId);
	statement->registerOutParam(2, OCCINUMBER);

	statement->execute();

	return statement->getNumber(2);
}

double findProduct(Connection* conn, int productId)
{
	Statement* statement = conn->createStatement("BEGIN find_product(:1,:2); END;");
	statement->setNumber(1, productId);
	statement->registerOutParam(2, OCCINUMBER);

	statement->execute();

	return statement->getNumber(2);
}

int addToCart(Connection* conn, struct ShoppingCart cart[])
{
	cout << "-------------- Add Products to Cart --------------" << endl;
	bool exit = false;
	int i = 0;
	for (i; i < 5 && !exit; i++)
	{
		int productId;
		double price = 0;
		do
		{
			do
			{
				if (cin.fail())
				{
					cout << "Product ID must be a number. ";
					cin.clear();
					cin.ignore(INT_MAX, '\n');
				}

				cout << "Enter the product ID: ";
				cin >> productId;
			} while (cin.fail());

			price = findProduct(conn, productId);
			if (price == 0)
				cout << "The product does not exists. Try again..." << endl;
		} while (price == 0);

		cout << "Product Price: " << price << endl;
		
		int quantity;
		do
		{
			if (cin.fail() || quantity < 1)
			{
				cout << "Quantity must be a positive number. ";
				cin.clear();
				cin.ignore(INT_MAX, '\n');
			}

			cout << "Enter the product Quantity: ";
			cin >> quantity;
		} while (cin.fail() || quantity < 1);

		if (i < 4)
		{
			int selection;
			do
			{
				if (cin.fail())
				{
					cin.clear();
					cin.ignore(INT_MAX, '\n');
				}

				cout << "Enter 1 to add more products or 0 to checkout: ";
				cin >> selection;
			} while (cin.fail() || (selection != 0 && selection != 1));

			if (selection == 0) exit = true;
		}

		cart[i].product_id = productId;
		cart[i].price = price;
		cart[i].quantity = quantity;
	}

	return i;
}

void displayProducts(struct ShoppingCart cart[], int productCount)
{
	cout << "------- Ordered Products ---------" << endl;
	for (int i = 0; i < productCount; i++)
	{
		cout
			<< "--Item " << i + 1 << endl
			<< "Product ID: " << cart[i].product_id << endl
			<< "Price: " << cart[i].price << endl
			<< "Quantity: " << cart[i].quantity << endl;
	}

	double total = accumulate(cart, cart + productCount, double(0), [](double sum, ShoppingCart cartItem) { return sum + (cartItem.price * double(cartItem.quantity)); });

	cout
		<< "----------------------------------" << endl
		<< "Total: " << total << endl;
}

void checkout(Connection* conn, struct ShoppingCart cart[], int customerId, int productCount)
{
	char checkoutChoice{};
	do
	{
		cout << "Would you like to checkout? (Y/y or N/n) ";
		cin >> checkoutChoice;
		checkoutChoice = toupper(checkoutChoice);
		if (checkoutChoice != 'Y' && checkoutChoice != 'N')
			cout << "Wrong input. Try again..." << endl;
	} while (checkoutChoice != 'Y' && checkoutChoice != 'N');

	if (checkoutChoice == 'N')
	{
		cout << "The order is cancelled." << endl;
		return;
	}
	
	Statement* statement = conn->createStatement("BEGIN add_order(:1,:2); END;");
	statement->setNumber(1, customerId);
	statement->registerOutParam(2, OCCINUMBER);

	statement->execute();

	int orderId = statement->getNumber(2);

	for (int i = 0; i < productCount; i++)
	{
		Statement* statement = conn->createStatement("BEGIN add_order_item(:1,:2,:3,:4,:5); END;");
		statement->setNumber(1, orderId);
		statement->setNumber(2, i + 1);
		statement->setNumber(3, cart[i].product_id);
		statement->setNumber(4, cart[i].quantity);
		statement->setDouble(5, cart[i].price);

		statement->execute();
	}

	cout << "The order is successfully completed." << endl;
}

int main() 
{
	try
	{
		Environment* env = Environment::createEnvironment(Environment::DEFAULT);
		Connection* connection = env->createConnection(user, password, connectionString);

		int mainMenuChoice = -1;
		while (mainMenuChoice != 0)
		{
			mainMenuChoice = mainManu();
			if (mainMenuChoice == 1)
			{
				int customerId = getCustomerIdInput(connection);
				if (customerLogin(connection, customerId) == 1)
				{
					struct ShoppingCart cart[5];
					int itemsInCart = addToCart(connection, cart);
					displayProducts(cart, itemsInCart);
					checkout(connection, cart, customerId, itemsInCart);
				}
				else
					cout << "The customer does not exist." << endl;
			}
		}

		env->terminateConnection(connection);
		Environment::terminateEnvironment(env);
	}
	catch (SQLException& sqlExcp)
	{
		cout << sqlExcp.getErrorCode() << ": " << sqlExcp.getMessage();
	}

	return 0;
}